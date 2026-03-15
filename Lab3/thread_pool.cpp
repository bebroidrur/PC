#include "thread_pool.h"

ThreadPool::ThreadPool() = default;

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