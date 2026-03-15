#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
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

private:
    std::vector<std::thread> workers_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> terminated_{false};
};
#endif
