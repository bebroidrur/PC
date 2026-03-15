#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

constexpr int MIN_TASK_TIME = 6;
constexpr int MAX_TASK_TIME = 12;
constexpr int QUEUE_COUNT = 3;
constexpr int WORKERS_PER_QUEUE = 2;
constexpr int MAX_QUEUE_SIZE = 10;
constexpr int GENERATOR_COUNT = 2;

struct Task {
    int id{};
    int durationSec{};
};

struct QueueFullStats {
    bool isFull{false};
    std::chrono::steady_clock::time_point fullStart{};
    std::vector<double> fullDurationsSec;
};

class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    void initialize();
    void terminate();

    void pause();
    void resume();
    [[nodiscard]] bool isPaused() const;

    bool submitTask(const Task& task);

    [[nodiscard]] std::size_t getQueueSize(int queueId) const;
    [[nodiscard]] std::size_t getTotalTasks() const;
    [[nodiscard]] int getRejectedTasks() const;
    [[nodiscard]] int getCompletedTasks() const;
    [[nodiscard]] int getWorkerCount() const;

    [[nodiscard]] double getTotalFullTime() const;
    [[nodiscard]] double getMinFullTime() const;
    [[nodiscard]] double getMaxFullTime() const;

private:
    bool tryPushToQueue(int queueId, const Task& task);
    bool tryPopFromQueue(int queueId, Task& task);
    void workerRoutine(int queueId);

    void onQueueStateAfterPush(int queueId);
    void onQueueStateAfterPop(int queueId);

private:
    mutable std::mutex queuesMutex_;
    mutable std::mutex coutMutex_;
    std::condition_variable taskAvailable_;

    std::vector<std::deque<Task>> queues_;
    std::vector<std::thread> workers_;
    std::vector<QueueFullStats> queueStats_;

    std::atomic<bool> initialized_{false};
    std::atomic<bool> terminated_{false};
    std::atomic<bool> paused_{false};

    std::atomic<int> rejectedTasks_{0};
    std::atomic<int> completedTasks_{0};
};
#endif