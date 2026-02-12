#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <fstream>

using namespace std;
using namespace std::chrono;

static void fillRandom(vector<int>& v, int lo = 0, int hi = 100) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<int> dist(lo, hi);
    for (auto &x : v) x = dist(gen);
}

static double computeSingle(const vector<int>& A, const vector<int>& B, int k, vector<int>& C) {
    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < A.size(); ++i) {
        C[i] = A[i] - k * B[i];
    }

    auto end = high_resolution_clock::now();
    auto elapsed = duration_cast<duration<double>>(end - start);
    return elapsed.count();
}

static void computeRange(const vector<int>& A, const vector<int>& B, int k,
                         vector<int>& C, size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
        C[i] = A[i] - k * B[i];
    }
}

static double computeParallel(const vector<int>& A, const vector<int>& B, int k,
                              vector<int>& C, size_t threadCount) {
    if (threadCount < 1) threadCount = 1;
    threadCount = min(threadCount, A.size());

    vector<thread> threads;
    threads.reserve(threadCount);

    size_t n = A.size();
    size_t chunk = n / threadCount;
    size_t rem = n % threadCount;

    auto start = high_resolution_clock::now();

    size_t cur = 0;
    for (size_t t = 0; t < threadCount; ++t) {
        size_t begin = cur;
        size_t end = begin + chunk + (t < rem ? 1 : 0);
        cur = end;

        threads.emplace_back(computeRange, cref(A), cref(B), k, ref(C), begin, end);
    }

    for (auto &th : threads) th.join();

    auto end = high_resolution_clock::now();
    auto elapsed = duration_cast<duration<double>>(end - start);
    return elapsed.count();
}

static bool equalVectors(const vector<int>& X, const vector<int>& Y) {
    return X.size() == Y.size() && equal(X.begin(), X.end(), Y.begin());
}

int main() {
    const vector<size_t> sizes = {1000, 5000, 10000, 50000, 100000, 500000, 1000000, 5000000};
    const vector<size_t> threadsList = {1, 2, 4, 8, 16, 32, 64, 128};
    const int k = 5;

    for (size_t N : sizes) {

        string fileName = "results_" + to_string(N) + ".csv";
        ofstream out(fileName);

        if (!out.is_open()) {
            cerr << "ERROR: cannot open " << fileName << "\n";
            continue;
        }

        out << "threads,time_seconds,check\n";

        vector<int> A(N), B(N), Csingle(N), Cpar(N);

        fillRandom(A);
        fillRandom(B);

        double tSingle = computeSingle(A, B, k, Csingle);
        out << 1 << "," << tSingle << ",OK\n";

        cout << "\nN=" << N << "\n";
        cout << "  single (1 thread): " << tSingle << " s\n";

        for (size_t th : threadsList) {

            if (th == 1) continue;

            double tPar = computeParallel(A, B, k, Cpar, th);
            bool ok = equalVectors(Csingle, Cpar);

            out << th << "," << tPar << ","
                << (ok ? "OK" : "FAIL") << "\n";

            cout << "  threads=" << th << ": " << tPar << " s"
                 << " | check=" << (ok ? "OK" : "FAIL") << "\n";
        }

        out.close();
    }

    return 0;
}

