#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

using namespace std;
using namespace chrono;

// 优化算法：逐行访问（行优先，cache友好）
__attribute__((noinline)) void row_major(const vector<double> &matrix, const vector<double> &vec,
                                         vector<double> &result, int n)
{
    // 初始化结果数组
    for (int i = 0; i < n; i++)
    {
        result[i] = 0.0;
    }

    // 按行访问，充分利用空间局部性
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            result[j] += matrix[i * n + j] * vec[i]; // 行访问，连续内存
        }
    }
}

int main()
{
    vector<int> sizes = {256, 512, 1024, 2048, 4096};
    int repeat_times = 100;

    cout << "=== 优化算法（行优先访问）===" << endl;
    cout << "矩阵规模N, 总时间(ms), 单次时间(ms)" << endl;

    for (int n : sizes)
    {
        size_t matrix_size = n * n * sizeof(double);
        cout << "N=" << n << " 矩阵大小: " << matrix_size / 1024 << " KB" << endl;

        vector<double> matrix(n * n);
        vector<double> vec(n);
        vector<double> result(n);

        for (int i = 0; i < n; i++)
        {
            vec[i] = 1.0;
            for (int j = 0; j < n; j++)
            {
                matrix[i * n + j] = i + j;
            }
        }

        volatile double dummy = 0.0;

        auto start = high_resolution_clock::now();

        for (int t = 0; t < repeat_times; t++)
        {
            row_major(matrix, vec, result, n);
            dummy = result[0];
        }

        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start);
        double avg_ms = duration_ms.count() / (double)repeat_times;

        cout << n << ", " << duration_ms.count() << ", "
             << fixed << setprecision(3) << avg_ms << endl;
    }

    return 0;
}