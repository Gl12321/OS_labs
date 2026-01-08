#include <iostream>
#include <vector>
#include <algorithm>
#include <pthread.h>
#include <chrono>

using namespace std;

// Структура для передачи аргументов в поток (умножение ровно ДВУХ блоков)
struct ThreadData {
    const vector<vector<int>>* A;
    const vector<vector<int>>* B;
    vector<vector<int>>* out;
    int row, col, inner, k;
    pthread_mutex_t* block_mutex; // Мьютекс, защищающий конкретный результирующий блок
};

class MatrixMultiplier {
public:
    // Потоковая функция: C_ij += A_im * B_mj
    static void* multiply_two_blocks_thread(void* arg) {
        ThreadData* d = static_cast<ThreadData*>(arg);
        int n = d->A->size();

        // Реальные границы (с учетом краевых элементов 3x1, 1x1 и т.д.)
        int i_end = min(d->row + d->k, n);
        int j_end = min(d->col + d->k, n);
        int k_end = min(d->inner + d->k, n);

        // 1. Вычисляем произведение двух блоков в локальный буфер.
        // Это "Best Practice" в системном программировании: вычисляем вне критической секции.
        vector<vector<int>> local_buf(i_end - d->row, vector<int>(j_end - d->col, 0));

        for (int i = d->row; i < i_end; i++) {
            for (int j = d->col; j < j_end; j++) {
                for (int l = d->inner; l < k_end; l++) {
                    local_buf[i - d->row][j - d->col] += (*d->A)[i][l] * (*d->B)[l][j];
                }
            }
        }

        // 2. Критическая секция: прибавляем локальный результат к общей матрице
        pthread_mutex_lock(d->block_mutex);
        for (int i = d->row; i < i_end; i++) {
            for (int j = d->col; j < j_end; j++) {
                (*d->out)[i][j] += local_buf[i - d->row][j - d->col];
            }
        }
        pthread_mutex_unlock(d->block_mutex);

        delete d; 
        return nullptr;
    }

    void parallelMultiply(vector<vector<int>>& A, vector<vector<int>>& B, 
                          vector<vector<int>>& out, int k) {
        int n = A.size();
        int grid_size = (n + k - 1) / k;

        // Инициализация сетки мьютексов
        vector<vector<pthread_mutex_t>> mutex_grid(grid_size, vector<pthread_mutex_t>(grid_size));
        for (int i = 0; i < grid_size; i++) {
            for (int j = 0; j < grid_size; j++) {
                pthread_mutex_init(&mutex_grid[i][j], nullptr);
            }
        }

        vector<pthread_t> threads;

        // Тройной цикл по схеме: порождаем поток на КАЖДУЮ ПАРУ БЛОКОВ
        for (int i = 0; i < n; i += k) {
            for (int j = 0; j < n; j += k) {
                for (int m = 0; m < n; m += k) {
                    ThreadData* data = new ThreadData{
                        &A, &B, &out, i, j, m, k, &mutex_grid[i/k][j/k]
                    };
                    pthread_t tid;
                    // Создание потока (интерфейс ядра через clone)
                    pthread_create(&tid, nullptr, multiply_two_blocks_thread, data);
                    threads.push_back(tid);
                }
            }
        }

        // Ждем все потоки
        for (pthread_t tid : threads) {
            pthread_join(tid, nullptr);
        }

        // Очистка мьютексов
        for (int i = 0; i < grid_size; i++) {
            for (int j = 0; j < grid_size; j++) {
                pthread_mutex_destroy(&mutex_grid[i][j]);
            }
        }
    }
};

int main() {
    int N = 100, K = 20; 
    MatrixMultiplier mm;

    vector<vector<int>> A(N, vector<int>(N, 1));
    vector<vector<int>> B(N, vector<int>(N, 2));
    vector<vector<int>> C(N, vector<int>(N, 0));

    auto start = chrono::steady_clock::now();
    mm.parallelMultiply(A, B, C, K);
    auto end = chrono::steady_clock::now();

    auto diff = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << diff.count() << endl;

    return 0;
}