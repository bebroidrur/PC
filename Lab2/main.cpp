#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

struct Result {
    long long countGreater = 0;
    int maxVal = std::numeric_limits<int>::min();
};

template <class Func>
double measure_ms(Func&& f) {
    auto t1 = std::chrono::high_resolution_clock::now();
    f();
    auto t2 = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(t2 - t1).count();
}

Result run_sequential(const std::vector<int>& a, int X) {
    Result r;
    for (int v : a) {
        if (v > X) r.countGreater++;
        if (v > r.maxVal) r.maxVal = v;
    }
    return r;
}

Result run_parallel_mutex_blocking(const std::vector<int>& a, int X, int numThreads) {
    Result global;
    std::mutex m;

    const size_t n = a.size();
    if (n == 0) return global;

    numThreads = std::max(1, numThreads);
    numThreads = std::min<int>(numThreads, static_cast<int>(n));

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    auto worker = [&](int tid) {
        const size_t chunk = (n + numThreads - 1) / numThreads;
        const size_t start = static_cast<size_t>(tid) * chunk;
        const size_t end = std::min(n, start + chunk);

        for (size_t i = start; i < end; ++i) {
            int v = a[i];
            std::lock_guard<std::mutex> lock(m);

            if (v > X) global.countGreater++;
            if (v > global.maxVal) global.maxVal = v;
        }
    };

    for (int t = 0; t < numThreads; ++t) threads.emplace_back(worker, t);
    for (auto& th : threads) th.join();

    return global;
}

Result run_parallel_atomic_cas(const std::vector<int>& a, int X, int numThreads) {
    const size_t n = a.size();
    if (n == 0) return {0, std::numeric_limits<int>::min()};

    numThreads = std::max(1, numThreads);
    numThreads = std::min<int>(numThreads, static_cast<int>(n));

    std::atomic<long long> countGreater{0};
    std::atomic<int> maxVal{std::numeric_limits<int>::min()};

    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    auto worker = [&](int tid) {
        const size_t chunk = (n + numThreads - 1) / numThreads;
        const size_t start = static_cast<size_t>(tid) * chunk;
        const size_t end = std::min(n, start + chunk);

        for (size_t i = start; i < end; ++i) {
            int v = a[i];

            if (v > X) {
                countGreater.fetch_add(1, std::memory_order_relaxed);
            }

            int cur = maxVal.load(std::memory_order_relaxed);
            while (v > cur && !maxVal.compare_exchange_weak(
                                  cur, v,
                                  std::memory_order_relaxed,
                                  std::memory_order_relaxed)) {
                                  }
        }
    };

    for (int t = 0; t < numThreads; ++t) threads.emplace_back(worker, t);
    for (auto& th : threads) th.join();

    return {countGreater.load(std::memory_order_relaxed),
            maxVal.load(std::memory_order_relaxed)};
}

int main() {
    const size_t N = 1'000'000;
    const int X = 10;
    const int numThreads = 6;

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(-100000, 100000);

    std::vector<int> a(N);
    for (size_t i = 0; i < N; ++i) a[i] = dist(rng);

    std::cout << "N=" << N << " X=" << X << "\n\n";

    Result seqRes;
    double seqTime = measure_ms([&] {
        seqRes = run_sequential(a, X);
    });

    Result parRes;
    double parTime = measure_ms([&] {
        parRes =  run_parallel_mutex_blocking(a, X, numThreads);
    });
    Result casRes;
    double casTime = measure_ms([&] { casRes = run_parallel_atomic_cas(a, X, numThreads); });

    std::cout << "Sequential:\n";
    std::cout << "  countGreater=" << seqRes.countGreater << " maxVal=" << seqRes.maxVal
              << " time_ms=" << seqTime << "\n\n";

    std::cout << "Parallel (mutex blocking):\n";
    std::cout << "  countGreater=" << parRes.countGreater << " maxVal=" << parRes.maxVal
              << " time_ms=" << parTime
              << ((seqRes.countGreater == parRes.countGreater && seqRes.maxVal == parRes.maxVal) ? " [OK]" : " [ERROR]")
              << "\n\n";

    std::cout << "Parallel (atomic + CAS):\n";
    std::cout << "  countGreater=" << casRes.countGreater << " maxVal=" << casRes.maxVal
              << " time_ms=" << casTime
              << ((seqRes.countGreater == casRes.countGreater && seqRes.maxVal == casRes.maxVal) ? " [OK]" : " [ERROR]")
              << "\n\n";

    if (parTime > 0) std::cout << "Speedup Seq/Mutex = " << (seqTime / parTime) << "\n";
    if (casTime > 0) std::cout << "Speedup Seq/CAS   = " << (seqTime / casTime) << "\n";

    return 0;
}