#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

struct Result {
    long long countGreater = 0;
    int maxVal = std::numeric_limits<int>::min();
};

Result run_sequential(const std::vector<int>& a, int X) {
    Result r;
    for (int v : a) {
        if (v > X) r.countGreater++;
        if (v > r.maxVal) r.maxVal = v;
    }
    return r;
}

int main() {
    const size_t N = 1'000'000;
    const int X = 10;

    std::mt19937 rng(123);
    std::uniform_int_distribution<int> dist(-100000, 100000);

    std::vector<int> a(N);
    for (size_t i = 0; i < N; ++i) a[i] = dist(rng);

    auto t1 = std::chrono::high_resolution_clock::now();
    Result r = run_sequential(a, X);
    auto t2 = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t2 - t1).count();

    std::cout << "Sequential\n";
    std::cout << "N=" << N << " X=" << X << "\n";
    std::cout << "countGreater=" << r.countGreater << "\n";
    std::cout << "maxVal=" << r.maxVal << "\n";
    std::cout << "time_ms=" << ms << "\n";
    return 0;
}