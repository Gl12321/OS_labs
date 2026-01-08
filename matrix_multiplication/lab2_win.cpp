#include <bits/stdc++.h>
#include <windows.h>
using namespace std;
using hrclock = chrono::high_resolution_clock;
using dbl = double;

vector<vector<dbl>> make_mat(int N, bool rnd = true) {
    vector<vector<dbl>> M(N, vector<dbl>(N));
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            M[i][j] = rnd ? ((rand() % 100) - 50) : 0;
    return M;
}

void mul_block(const vector<vector<dbl>> &A, const vector<vector<dbl>> &B,
               vector<vector<dbl>> &C, int N, int ai, int aj, int bi, int bj, int br) {
    for (int i = 0; i < br && ai + i < N; i++)
        for (int k = 0; k < br && aj + k < N; k++)
            for (int j = 0; j < br && bj + j < N; j++)
                C[ai + i][bj + j] += A[ai + i][aj + k] * B[bi + k][bj + j];
}

struct Task {
    const vector<vector<dbl>> *A, *B;
    vector<vector<dbl>> *C;
    int N, br, ai, aj, bi, bj;
};
DWORD WINAPI worker(LPVOID arg) {
    Task *t = (Task *)arg;
    mul_block(*t->A, *t->B, *t->C, t->N, t->ai, t->aj, t->bi, t->bj, t->br);
    delete t;
    return 0;
}

void run_winapi(const vector<vector<dbl>> &A, const vector<vector<dbl>> &B,
                vector<vector<dbl>> &C, int N, int br, int max_threads) {
    int nb = (N + br - 1) / br;
    vector<HANDLE> handles;
    for (int bi = 0; bi < nb; bi++)
        for (int bj = 0; bj < nb; bj++)
            for (int bk = 0; bk < nb; bk++) {
                Task *t = new Task{&A, &B, &C, N, br, bi * br, bk * br, bk * br, bj * br};
                HANDLE h = CreateThread(nullptr, 0, worker, t, 0, nullptr);
                handles.push_back(h);
                if ((int)handles.size() >= max_threads) {
                    WaitForMultipleObjects((DWORD)handles.size(), handles.data(), TRUE, INFINITE);
                    for (auto &x : handles) CloseHandle(x);
                    handles.clear();
                }
            }
    if (!handles.empty()) {
        WaitForMultipleObjects((DWORD)handles.size(), handles.data(), TRUE, INFINITE);
        for (auto &x : handles) CloseHandle(x);
    }
}

void run_std_threads(const vector<vector<dbl>> &A, const vector<vector<dbl>> &B,
                     vector<vector<dbl>> &C, int N, int br, int max_threads) {
    int nb = (N + br - 1) / br;
    vector<thread> th;
    for (int bi = 0; bi < nb; bi++)
        for (int bj = 0; bj < nb; bj++)
            for (int bk = 0; bk < nb; bk++) {
                th.emplace_back([&, bi, bj, bk]() {
                    mul_block(A, B, C, N, bi * br, bk * br, bk * br, bj * br, br);
                });
                if ((int)th.size() >= max_threads) {
                    for (auto &t : th) t.join();
                    th.clear();
                }
            }
    for (auto &t : th) t.join();
}

void naive(const vector<vector<dbl>> &A, const vector<vector<dbl>> &B, vector<vector<dbl>> &C, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            dbl s = 0;
            for (int k = 0; k < N; k++) s += A[i][k] * B[k][j];
            C[i][j] = s;
        }
}

int main() {
    srand(1);
    int N = 100;
    int br = 20;
    int max_threads = 8;
    auto A = make_mat(N);
    auto B = make_mat(N);
    vector<vector<dbl>> C(N, vector<dbl>(N, 0));

    auto t0 = hrclock::now();
    naive(A, B, C, N);
    cout << "Naive time: " << chrono::duration<double, milli>(hrclock::now() - t0).count() << " ms\n";

    C.assign(N, vector<dbl>(N, 0));
    t0 = hrclock::now();
    run_std_threads(A, B, C, N, br, max_threads);
    cout << "std::thread time: " << chrono::duration<double, milli>(hrclock::now() - t0).count() << " ms\n";

    C.assign(N, vector<dbl>(N, 0));
    t0 = hrclock::now();
    run_winapi(A, B, C, N, br, max_threads);
    cout << "WinAPI time: " << chrono::duration<double, milli>(hrclock::now() - t0).count() << " ms\n";
}

