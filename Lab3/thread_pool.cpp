#include "thread_pool.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>

ThreadPool::ThreadPool() : queues_(QUEUE_COUNT) {}

ThreadPool::~ThreadPool() {
    terminate();
}

void ThreadPool::initialize() {
    if (initialized_) {
        return;
    }

    initialized_ = true;
}

void ThreadPool::terminate() {
    if (!initialized_ || terminated_) {
        return;
    }

    terminated_ = true;

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
    initialized_ = false;
}

bool ThreadPool::tryPushToQueue(int queueId, const Task& task) {
    if (queueId < 0 || queueId >= QUEUE_COUNT) {
        return false;
    }

    if (queues_[queueId].size() >= MAX_QUEUE_SIZE) {
        return false;
    }

    queues_[queueId].push_back(task);
    return true;
}

bool ThreadPool::submitTask(const Task& task) {
    if (!initialized_ || terminated_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(queuesMutex_);

    std::vector<int> indices(QUEUE_COUNT);
    std::iota(indices.begin(), indices.end(), 0);

    std::mt19937 gen(static_cast<unsigned>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::shuffle(indices.begin(), indices.end(), gen);

    for (int queueId : indices) {
        if (tryPushToQueue(queueId, task)) {
            std::cout << "Task " << task.id
                      << " added to queue " << queueId
                      << ", current size = " << queues_[queueId].size()
                      << '\n';
            return true;
        }
    }

    ++rejectedTasks_;
    std::cout << "Task " << task.id << " rejected: all queues are full\n";
    return false;
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