#include <iostream>
#include "thread_pool.h"

int main() {
    std::cout << "Lab 3 - Thread Pool Variant 18\n";
    std::cout << "Queues: " << QUEUE_COUNT << '\n';
    std::cout << "Workers per queue: " << WORKERS_PER_QUEUE << '\n';
    std::cout << "Max queue size: " << MAX_QUEUE_SIZE << '\n';
    std::cout << "Task duration: from " << MIN_TASK_TIME
              << " to " << MAX_TASK_TIME << " seconds\n";

    ThreadPool pool;
    pool.initialize();
    pool.terminate();

    return 0;
}