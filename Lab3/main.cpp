#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "thread_pool.h"

void generatorRoutine(ThreadPool& pool, int generatorId,
                      std::atomic<bool>& stopFlag,
                      std::atomic<int>& nextTaskId) {
    thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> taskTimeDist(MIN_TASK_TIME, MAX_TASK_TIME);
    std::uniform_int_distribution<int> pauseDist(500, 1500);

    while (!stopFlag.load()) {
        Task task;
        task.id = nextTaskId.fetch_add(1);
        task.durationSec = taskTimeDist(gen);

        std::cout << "Generator " << generatorId
                  << " created task " << task.id
                  << " with duration " << task.durationSec << " sec\n";

        pool.submitTask(task);

        std::this_thread::sleep_for(std::chrono::milliseconds(pauseDist(gen)));
    }
}

int main() {
    std::cout << "Lab 3 - Thread Pool Variant 18\n";
    std::cout << "Queues: " << QUEUE_COUNT << '\n';
    std::cout << "Workers per queue: " << WORKERS_PER_QUEUE << '\n';
    std::cout << "Max queue size: " << MAX_QUEUE_SIZE << '\n';
    std::cout << "Task duration: from " << MIN_TASK_TIME
              << " to " << MAX_TASK_TIME << " seconds\n";
    std::cout << "Generator threads: " << GENERATOR_COUNT << "\n\n";

    ThreadPool pool;
    pool.initialize();

    std::atomic<bool> stopFlag{false};
    std::atomic<int> nextTaskId{0};

    std::vector<std::thread> generators;
    for (int i = 0; i < GENERATOR_COUNT; ++i) {
        generators.emplace_back(generatorRoutine,
                                std::ref(pool),
                                i,
                                std::ref(stopFlag),
                                std::ref(nextTaskId));
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));
    stopFlag = true;

    for (auto& generator : generators) {
        if (generator.joinable()) {
            generator.join();
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "\nCurrent queue state:\n";
    for (int i = 0; i < QUEUE_COUNT; ++i) {
        std::cout << "Queue " << i << ": " << pool.getQueueSize(i) << " tasks\n";
    }

    std::cout << "Total tasks in queues: " << pool.getTotalTasks() << '\n';
    std::cout << "Rejected tasks: " << pool.getRejectedTasks() << '\n';

    pool.terminate();
    return 0;
}