#include "thread_pool.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>

ThreadPool::ThreadPool()
    : queues_(QUEUE_COUNT),
      queueStats_(QUEUE_COUNT) {}

ThreadPool::~ThreadPool() {
    terminate();
}

void ThreadPool::initialize() {
    if (initialized_) {
        return;
    }

    terminated_ = false;
    paused_ = false;

    for (int queueId = 0; queueId < QUEUE_COUNT; ++queueId) {
        for (int i = 0; i < WORKERS_PER_QUEUE; ++i) {
            workers_.emplace_back(&ThreadPool::workerRoutine, this, queueId);
        }
    }

    initialized_ = true;
}

void ThreadPool::terminate() {
    if (!initialized_ || terminated_) {
        return;
    }

    terminated_ = true;
    taskAvailable_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    initialized_ = false;
}

void ThreadPool::pause() {
    paused_ = true;

    std::lock_guard<std::mutex> coutLock(coutMutex_);
    std::cout << "\n=== THREAD POOL PAUSED ===\n";
}

void ThreadPool::resume() {
    paused_ = false;

    {
        std::lock_guard<std::mutex> coutLock(coutMutex_);
        std::cout << "\n=== THREAD POOL RESUMED ===\n";
    }

    taskAvailable_.notify_all();
}

bool ThreadPool::isPaused() const {
    return paused_.load();
}

void ThreadPool::onQueueStateAfterPush(int queueId) {
    if (queueId < 0 || queueId >= QUEUE_COUNT) {
        return;
    }

    if (queues_[queueId].size() == MAX_QUEUE_SIZE && !queueStats_[queueId].isFull) {
        queueStats_[queueId].isFull = true;
        queueStats_[queueId].fullStart = std::chrono::steady_clock::now();
    }
}

void ThreadPool::onQueueStateAfterPop(int queueId) {
    if (queueId < 0 || queueId >= QUEUE_COUNT) {
        return;
    }

    if (queueStats_[queueId].isFull && queues_[queueId].size() < MAX_QUEUE_SIZE) {
        const auto fullEnd = std::chrono::steady_clock::now();
        const double durationSec =
            std::chrono::duration<double>(fullEnd - queueStats_[queueId].fullStart).count();

        queueStats_[queueId].fullDurationsSec.push_back(durationSec);
        queueStats_[queueId].isFull = false;
    }
}

bool ThreadPool::tryPushToQueue(int queueId, const Task& task) {
    if (queueId < 0 || queueId >= QUEUE_COUNT) {
        return false;
    }

    if (queues_[queueId].size() >= MAX_QUEUE_SIZE) {
        return false;
    }

    queues_[queueId].push_back(task);
    onQueueStateAfterPush(queueId);
    return true;
}

bool ThreadPool::tryPopFromQueue(int queueId, Task& task) {
    if (queueId < 0 || queueId >= QUEUE_COUNT) {
        return false;
    }

    if (queues_[queueId].empty()) {
        return false;
    }

    task = queues_[queueId].front();
    queues_[queueId].pop_front();
    onQueueStateAfterPop(queueId);
    return true;
}

bool ThreadPool::submitTask(const Task& task) {
    if (!initialized_ || terminated_) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(queuesMutex_);

        std::vector<int> indices(QUEUE_COUNT);
        std::iota(indices.begin(), indices.end(), 0);

        std::mt19937 gen(static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        std::shuffle(indices.begin(), indices.end(), gen);

        for (int queueId : indices) {
            if (tryPushToQueue(queueId, task)) {
                {
                    std::lock_guard<std::mutex> coutLock(coutMutex_);
                    std::cout << "Task " << task.id
                              << " added to queue " << queueId
                              << ", current size = " << queues_[queueId].size()
                              << '\n';
                }

                taskAvailable_.notify_one();
                return true;
            }
        }
    }

    ++rejectedTasks_;

    {
        std::lock_guard<std::mutex> coutLock(coutMutex_);
        std::cout << "Task " << task.id << " rejected: all queues are full\n";
    }

    return false;
}

void ThreadPool::workerRoutine(int queueId) {
    while (true) {
        Task task;

        {
            std::unique_lock<std::mutex> lock(queuesMutex_);

            taskAvailable_.wait(lock, [this, queueId] {
                return terminated_ || (!paused_ && !queues_[queueId].empty());
            });

            if (terminated_ && queues_[queueId].empty()) {
                return;
            }

            if (paused_) {
                continue;
            }

            if (!tryPopFromQueue(queueId, task)) {
                continue;
            }
        }

        {
            std::lock_guard<std::mutex> coutLock(coutMutex_);
            std::cout << "Worker from queue " << queueId
                      << " started task " << task.id
                      << " for " << task.durationSec << " sec\n";
        }

        std::this_thread::sleep_for(std::chrono::seconds(task.durationSec));

        ++completedTasks_;

        {
            std::lock_guard<std::mutex> coutLock(coutMutex_);
            std::cout << "Worker from queue " << queueId
                      << " completed task " << task.id << '\n';
        }
    }
}

std::size_t ThreadPool::getQueueSize(int queueId) const {
    std::lock_guard<std::mutex> lock(queuesMutex_);

    if (queueId < 0 || queueId >= QUEUE_COUNT) {
        return 0;
    }

    return queues_[queueId].size();
}

std::size_t ThreadPool::getTotalTasks() const {
    std::lock_guard<std::mutex> lock(queuesMutex_);

    std::size_t total = 0;
    for (const auto& queue : queues_) {
        total += queue.size();
    }
    return total;
}

int ThreadPool::getRejectedTasks() const {
    return rejectedTasks_.load();
}

int ThreadPool::getCompletedTasks() const {
    return completedTasks_.load();
}

int ThreadPool::getWorkerCount() const {
    return QUEUE_COUNT * WORKERS_PER_QUEUE;
}

double ThreadPool::getTotalFullTime() const {
    std::lock_guard<std::mutex> lock(queuesMutex_);

    double total = 0.0;
    for (const auto& stat : queueStats_) {
        for (double duration : stat.fullDurationsSec) {
            total += duration;
        }
    }
    return total;
}

double ThreadPool::getMinFullTime() const {
    std::lock_guard<std::mutex> lock(queuesMutex_);

    double minValue = std::numeric_limits<double>::max();
    bool found = false;

    for (const auto& stat : queueStats_) {
        for (double duration : stat.fullDurationsSec) {
            minValue = std::min(minValue, duration);
            found = true;
        }
    }

    return found ? minValue : 0.0;
}

double ThreadPool::getMaxFullTime() const {
    std::lock_guard<std::mutex> lock(queuesMutex_);

    double maxValue = 0.0;

    for (const auto& stat : queueStats_) {
        for (double duration : stat.fullDurationsSec) {
            maxValue = std::max(maxValue, duration);
        }
    }

    return maxValue;
}