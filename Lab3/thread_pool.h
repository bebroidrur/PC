#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#pragma once

#include <atomic>
#include <deque>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

constexpr int MIN_TASK_TIME = 6;
constexpr int MAX_TASK_TIME = 12;
constexpr int QUEUE_COUNT = 3;
constexpr int WORKERS_PER_QUEUE = 2;
constexpr int MAX_QUEUE_SIZE = 10;

struct Task {
    int id{};
    int durationSec{};
};

class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    void initialize();
    void terminate();

    bool submitTask(const Task& task);

    [[nodiscard]] std::size_t getQueueSize(int queueId) const;
    [[nodiscard]] std::size_t getTotalTasks() const;
    [[nodiscard]] int getRejectedTasks() const;

private:
    bool tryPushToQueue(int queueId, const Task& task);
    bool tryPopFromQueue(int queueId, Task& task);
    void workerRoutine(int queueId);
private:
    mutable std::mutex queuesMutex_;
    std::condition_variable taskAvailable_;

    std::vector<std::deque<Task>> queues_;
    std::vector<std::thread> workers_;

    std::atomic<bool> initialized_{false};
    std::atomic<bool> terminated_{false};
    std::atomic<int> rejectedTasks_{0};
};;
#endif
