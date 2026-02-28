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

    std::cout << "Sequential:\n";
    std::cout << "  countGreater=" << seqRes.countGreater << "\n";
    std::cout << "  maxVal=" << seqRes.maxVal << "\n";
    std::cout << "  time_ms=" << seqTime << "\n\n";

    Result parRes;
    double parTime = measure_ms([&] {
        parRes =  run_parallel_mutex_blocking(a, X, numThreads);
    });

    std::cout << "Parallel (mutex blocking):\n";
    std::cout << "  countGreater=" << parRes.countGreater << "\n";
    std::cout << "  maxVal=" << parRes.maxVal << "\n";
    std::cout << "  time_ms=" << parTime << "\n\n";
    if (seqRes.countGreater == parRes.countGreater &&
        seqRes.maxVal == parRes.maxVal) {
        std::cout << "Results match [OK]\n";
        } else {
            std::cout << "Results mismatch [ERROR]\n";
        }

    return 0;
}