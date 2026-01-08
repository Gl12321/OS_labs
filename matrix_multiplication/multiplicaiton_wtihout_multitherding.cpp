#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>

using namespace std;

class MatrixMultiplier {
public:
    void MultiplyTwoBlocks(const vector<vector<int>>& A, const vector<vector<int>>& B, vector<vector<int>>& out,
                           int row, int col, int inner, int k) {
        int n = A.size();
        
        int i_end = min(row + k, n);
        int j_end = min(col + k, n);
        int k_end = min(inner + k, n);

        for (int i = row; i < i_end; i++) {
            for (int j = col; j < j_end; j++) {
                for (int l = inner; l < k_end; l++) {
                    out[i][j] += A[i][l] * B[l][j];
                }
            }
        }
    }

    void sequentialMultiply(vector<vector<int>>& A, vector<vector<int>>& B, vector<vector<int>>& out, int k) {
        int n = A.size();
        
        for (int i = 0; i < n; i += k) {
            for (int j = 0; j < n; j += k) {
                for (int m = 0; m < n; m += k) {
                    MultiplyTwoBlocks(A, B, out, i, j, m, k);
                }
            }
        }
    }
};

int main() {
    int N = 100;   
    int K = 20;    
    
    MatrixMultiplier mm;

    vector<vector<int>> A(N, vector<int>(N, 1));
    vector<vector<int>> B(N, vector<int>(N, 2));
    vector<vector<int>> C(N, vector<int>(N, 0));

    auto start = chrono::steady_clock::now();

    mm.sequentialMultiply(A, B, C, K);

    auto end = chrono::steady_clock::now();
    auto diff = chrono::duration_cast<chrono::microseconds>(end - start);

    cout << diff.count() << endl;

    return 0;
}