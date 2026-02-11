#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

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
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);

    return elapsed.count();
}

int main() {
    const size_t N = 1000000;
    const int k = 5;

    vector<int> A(N), B(N), C(N);

    fillRandom(A);
    fillRandom(B);

    double t1 = computeSingle(A, B, k, C);
    cout << "Single-thread time: " << t1 << " seconds\n";

    cout << "First 10 C values: ";
    for (int i = 0; i < 10; ++i) cout << C[i] << ' ';
    cout << "\n";

    return 0;
}