#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <mutex>
#include <memory>

using namespace std;

class MatrixMultiplier {
public:
    void multiply_task(const vector<vector<int>>& A, const vector<vector<int>>& B, 
                       vector<vector<int>>& out, int row, int col, int inner, 
                       int k, mutex& mtx) {
        int n = A.size();
        int i_e = min(row + k, n), j_e = min(col + k, n), k_e = min(inner + k, n);
        vector<vector<int>> res(i_e - row, vector<int>(j_e - col, 0));

        for (int i = row; i < i_e; i++)
            for (int j = col; j < j_e; j++)
                for (int l = inner; l < k_e; l++)
                    res[i - row][j - col] += A[i][l] * B[l][j];

        lock_guard<mutex> lock(mtx);
        for (int i = row; i < i_e; i++)
            for (int j = col; j < j_e; j++)
                out[i][j] += res[i - row][j - col];
    }

    void parallelMultiply(vector<vector<int>>& A, vector<vector<int>>& B, 
                          vector<vector<int>>& out, int k) {
        int n = A.size();
        int gs = (n + k - 1) / k;
        vector<vector<unique_ptr<mutex>>> m_grid(gs);
        for (int i = 0; i < gs; i++)
            for (int j = 0; j < gs; j++)
                m_grid[i].emplace_back(make_unique<mutex>());

        vector<future<void>> futures;
        for (int i = 0; i < n; i += k)
            for (int j = 0; j < n; j += k)
                for (int m = 0; m < n; m += k)
                    futures.push_back(async(launch::async, &MatrixMultiplier::multiply_task, this, 
                                      ref(A), ref(B), ref(out), i, j, m, k, ref(*m_grid[i/k][j/k])));
        
        for (auto& f : futures) f.wait();
    }
};

int main() {
    int N = 100, K = 20;
    MatrixMultiplier mm;

    vector<vector<int>> A(N, vector<int>(N, 1)), B(N, vector<int>(N, 2)), C(N, vector<int>(N, 0));

    auto start = chrono::steady_clock::now();
    mm.parallelMultiply(A, B, C, K);
    auto end = chrono::steady_clock::now();

    cout << chrono::duration_cast<chrono::milliseconds>(end - start).count() << endl;
    
    return 0;
}