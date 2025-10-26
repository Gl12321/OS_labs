
#include <bits/stdc++.h>
#include <chrono>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#ifdef _WIN32
  #define HAVE_WINAPI 1
  #include <windows.h>
#else
  #define HAVE_WINAPI 0
#endif

#if !defined(_WIN32)
  #define HAVE_PTHREAD 1
  #include <pthread.h>
#else
  #define HAVE_PTHREAD 0
#endif

using namespace std;
using hrclock = chrono::high_resolution_clock;
using dbl = double;

class Semaphore {
    mutex m;
    condition_variable cv;
    size_t count;
public:
    Semaphore(size_t c): count(c) {}
    void acquire() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&]{ return count>0; });
        --count;
    }
    void release() {
        {
            lock_guard<mutex> lk(m);
            ++count;
        }
        cv.notify_one();
    }
};

vector<vector<dbl>> make_mat(int N, bool random=true) {
    vector<vector<dbl>> M(N, vector<dbl>(N));
    for(int i=0;i<N;i++) for(int j=0;j<N;j++) M[i][j] = random ? ((rand()%100)-50) : 0.0;
    return M;
}

void add_block_to_C(vector<vector<dbl>>& C, int N,
                    const vector<vector<dbl>>& block, int di, int dj,
                    int br, mutex &cblock_mutex)
{
    lock_guard<mutex> lk(cblock_mutex);
    for(int i=0;i<br && di+i<N;i++){
        for(int j=0;j<br && dj+j<N;j++){
            C[di+i][dj+j] += block[i][j];
        }
    }
}

void mul_block_to_local(const vector<vector<dbl>>& A, const vector<vector<dbl>>& B,
                        int N, int ai, int aj, int bi, int bj, int br,
                        vector<vector<dbl>>& out)
{
    for(int i=0;i<br && ai+i<N;i++){
        for(int k=0;k<br && aj+k<N;k++){
            dbl av = A[ai+i][aj+k];
            for(int j=0;j<br && bj+j<N;j++){
                out[i][j] += av * B[bi+k][bj+j];
            }
        }
    }
}

void run_std_threads(const vector<vector<dbl>>& A, const vector<vector<dbl>>& B,
                     vector<vector<dbl>>& C, int N, int br, size_t max_concurrency)
{
    int nb = (N + br - 1) / br;
    Semaphore sem(max_concurrency==0 ? thread::hardware_concurrency() : max_concurrency);
    vector<mutex> cblock_mutex(nb*nb);
    vector<thread> workers;
    workers.reserve(nb*nb*nb);

    for(int bi=0; bi<nb; ++bi){
        for(int bj=0; bj<nb; ++bj){
            for(int bt=0; bt<nb; ++bt){
                sem.acquire();
                workers.emplace_back([&, bi, bj, bt]() {
                    int ai = bi*br;
                    int aj = bt*br;
                    int bi_idx = bt*br;
                    int bj_idx = bj*br;
                    int real_br_i = min(br, N - ai);
                    int real_br_k = min(br, N - aj);
                    int real_br_j = min(br, N - bj_idx);

                    vector<vector<dbl>> local(br, vector<dbl>(br, 0.0));
                    mul_block_to_local(A, B, N, ai, aj, bi_idx, bj_idx, br, local);

                    int cidx = bi*nb + bj;
                    add_block_to_C(C, N, local, ai, bj_idx, br, cblock_mutex[cidx]);

                    sem.release();
                });
            }
        }
    }
    for(auto &t : workers) if(t.joinable()) t.join();
}

#if HAVE_PTHREAD
struct PTask {
    const vector<vector<dbl>> *A,*B;
    vector<vector<dbl>> *C;
    int N, br;
    int bi,bj,bt, nb;
    vector<mutex> *cblock_mutex_ptr;
    PTask(const vector<vector<dbl>>* A_, const vector<vector<dbl>>* B_, vector<vector<dbl>>* C_,
          int N_, int br_, int bi_, int bj_, int bt_, int nb_, vector<mutex>* mptr)
      : A(A_), B(B_), C(C_), N(N_), br(br_), bi(bi_), bj(bj_), bt(bt_), nb(nb_), cblock_mutex_ptr(mptr) {}
};

void* pthread_worker(void* arg){
    PTask *task = (PTask*)arg;
    const auto &A = *task->A;
    const auto &B = *task->B;
    auto &C = *task->C;
    int N = task->N;
    int br = task->br;
    int ai = task->bi * br;
    int aj = task->bt * br;
    int bi_idx = task->bt * br;
    int bj_idx = task->bj * br;

    vector<vector<dbl>> local(br, vector<dbl>(br, 0.0));
    mul_block_to_local(A,B,N, ai, aj, bi_idx, bj_idx, br, local);

    int cidx = task->bi * task->nb + task->bj;
    add_block_to_C(C, N, local, ai, bj_idx, br, (*task->cblock_mutex_ptr)[cidx]);

    delete task;
    return nullptr;
}

void run_pthreads(const vector<vector<dbl>>& A, const vector<vector<dbl>>& B,
                  vector<vector<dbl>>& C, int N, int br, size_t max_concurrency)
{
    int nb = (N + br - 1) / br;
    vector<mutex> cblock_mutex(nb*nb);
    vector<pthread_t> threads;
    threads.reserve(nb*nb*nb);
    atomic<int> running{0};
    int limit = max_concurrency==0 ? thread::hardware_concurrency() : (int)max_concurrency;
    for(int bi=0; bi<nb; ++bi){
        for(int bj=0; bj<nb; ++bj){
            for(int bt=0; bt<nb; ++bt){
                while(running.load() >= limit) this_thread::sleep_for(chrono::microseconds(100));
                ++running;
                PTask *pt = new PTask(&A,&B,&C,N,br,bi,bj,bt,nb,&cblock_mutex);
                pthread_t tid;
                if(pthread_create(&tid, nullptr, pthread_worker, pt)!=0){
                    delete pt;
                    --running;
                    throw runtime_error("pthread_create failed");
                }
                threads.push_back(tid);

                --running;
            }
        }
    }
    for(auto &t : threads) pthread_join(t, nullptr);
}
#endif

#if HAVE_WINAPI
struct WinTask {
    const vector<vector<dbl>> *A,*B;
    vector<vector<dbl>> *C;
    int N, br;
    int bi,bj,bt, nb;
    vector<CRITICAL_SECTION> *crit_ptr;
};
DWORD WINAPI win_worker(LPVOID arg){
    WinTask *task = (WinTask*)arg;
    const auto &A = *task->A;
    const auto &B = *task->B;
    auto &C = *task->C;
    int N = task->N;
    int br = task->br;
    int ai = task->bi * br;
    int aj = task->bt * br;
    int bi_idx = task->bt * br;
    int bj_idx = task->bj * br;

    vector<vector<dbl>> local(br, vector<dbl>(br, 0.0));
    mul_block_to_local(A,B,N, ai, aj, bi_idx, bj_idx, br, local);

    int cidx = task->bi * task->nb + task->bj;
    // convert local to C with CRITICAL_SECTION
    EnterCriticalSection(&((*task->crit_ptr)[cidx]));
    for(int i=0;i<br && ai+i<N;i++){
        for(int j=0;j<br && bj_idx+j<N;j++){
            C[ai+i][bj_idx+j] += local[i][j];
        }
    }
    LeaveCriticalSection(&((*task->crit_ptr)[cidx]));
    delete task;
    return 0;
}

void run_winapi(const vector<vector<dbl>>& A, const vector<vector<dbl>>& B,
                vector<vector<dbl>>& C, int N, int br, size_t max_concurrency)
{
    int nb = (N + br - 1) / br;
    vector<CRITICAL_SECTION> crit(nb*nb);
    for(auto &cs : crit) InitializeCriticalSection(&cs);
    vector<HANDLE> handles;
    handles.reserve(nb*nb*nb);

    size_t limit = max_concurrency==0 ? thread::hardware_concurrency() : max_concurrency;
    Semaphore sem(limit);

    for(int bi=0; bi<nb; ++bi){
        for(int bj=0; bj<nb; ++bj){
            for(int bt=0; bt<nb; ++bt){
                sem.acquire();
                WinTask *wt = new WinTask{&A,&B,&C,N,br,bi,bj,bt,nb,&crit};
                HANDLE h = CreateThread(NULL, 0, win_worker, wt, 0, NULL);
                if(!h){
                    delete wt;
                    sem.release();
                    throw runtime_error("CreateThread failed");
                }
                thread([h,&sem]() {
                    WaitForSingleObject(h, INFINITE);
                    CloseHandle(h);
                    sem.release();
                }).detach();
                handles.push_back(h);
            }
        }
    }
    this_thread::sleep_for(chrono::milliseconds(100));
    for(auto &cs : crit) DeleteCriticalSection(&cs);
}
#endif

void naive_mul(const vector<vector<dbl>>& A, const vector<vector<dbl>>& B, vector<vector<dbl>>& C, int N){
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            dbl s = 0;
            for(int k=0;k<N;k++) s += A[i][k]*B[k][j];
            C[i][j] = s;
        }
    }
}

int main(int argc, char** argv){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    srand(12345);

    int N = 200;
    if(argc>1) N = atoi(argv[1]);
    if(N < 5) N = 5;
    size_t max_concurrency = 0; 
    if(argc>2) max_concurrency = (size_t)atoi(argv[2]);

    cerr << "N="<<N<<", max_concurrency="<<(max_concurrency==0?thread::hardware_concurrency():max_concurrency)<<"\n";

    auto A = make_mat(N, true);
    auto B = make_mat(N, true);

    vector<vector<dbl>> C_naive(N, vector<dbl>(N,0.0));
    auto t0 = hrclock::now();
    naive_mul(A,B,C_naive,N);
    auto t1 = hrclock::now();
    double naive_ms = chrono::duration<double, milli>(t1-t0).count();
    cout << "naive_time_ms " << naive_ms << "\n";

    int max_test_k = min(N, 20);
    for(int k = 1; k <= max_test_k; ++k){
        int br = k;
        int nb = (N + br - 1) / br;
        size_t tasks = (size_t)nb*nb*nb;
        cout << "block_size " << br << " blocks_per_dim " << nb << " tasks " << tasks << "\n";

        {
            vector<vector<dbl>> C(N, vector<dbl>(N,0.0));
            auto s = hrclock::now();
            try{
                run_std_threads(A,B,C,N,br,max_concurrency);
            } catch(exception &e){
                cerr << "std backend failed: " << e.what() << "\n";
            }
            auto e = hrclock::now();
            double ms = chrono::duration<double,milli>(e-s).count();
            cout << "std_time_ms " << br << " " << ms << "\n";
        }

#if HAVE_PTHREAD
        {
            vector<vector<dbl>> C(N, vector<dbl>(N,0.0));
            auto s = hrclock::now();
            try{
                run_pthreads(A,B,C,N,br,max_concurrency);
            } catch(exception &e){
                cerr << "pthread backend failed: " << e.what() << "\n";
            }
            auto e = hrclock::now();
            double ms = chrono::duration<double,milli>(e-s).count();
            cout << "pthread_time_ms " << br << " " << ms << "\n";
        }
#endif

#if HAVE_WINAPI
        {
            vector<vector<dbl>> C(N, vector<dbl>(N,0.0));
            auto s = hrclock::now();
            try{
                run_winapi(A,B,C,N,br,max_concurrency);
            } catch(exception &e){
                cerr << "winapi backend failed: " << e.what() << "\n";
            }
            auto e = hrclock::now();
            double ms = chrono::duration<double,milli>(e-s).count();
            cout << "winapi_time_ms " << br << " " << ms << "\n";
        }
#endif
    }

    cout << "done\n";
    return 0;
}
