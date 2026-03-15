#include <iostream>
#include "thread_pool.h"

int main() {
    std::cout << "Lab 3 - Thread Pool Variant 18\n";
    std::cout << "Queues: " << QUEUE_COUNT << '\n';
    std::cout << "Workers per queue: " << WORKERS_PER_QUEUE << '\n';
    std::cout << "Max queue size: " << MAX_QUEUE_SIZE << '\n';
    std::cout << "Task duration: from " << MIN_TASK_TIME
              << " to " << MAX_TASK_TIME << " seconds\n\n";

    ThreadPool pool;
    pool.initialize();

    for (int i = 0; i < 15; ++i) {
        Task task{i, MIN_TASK_TIME + (i % (MAX_TASK_TIME - MIN_TASK_TIME + 1))};
        pool.submitTask(task);
    }

    std::this_thread::sleep_for(std::chrono::seconds(20));

    std::cout << "\nCurrent queue state:\n";
    for (int i = 0; i < QUEUE_COUNT; ++i) {
        std::cout << "Queue " << i << ": " << pool.getQueueSize(i) << " tasks\n";
    }

    std::cout << "Total tasks in queues: " << pool.getTotalTasks() << '\n';
    std::cout << "Rejected tasks: " << pool.getRejectedTasks() << '\n';

    pool.terminate();
    return 0;
}