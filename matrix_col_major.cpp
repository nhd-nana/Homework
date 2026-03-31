#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

using namespace std;
using namespace chrono;

// 平凡算法：逐列访问（列优先，cache不友好）
__attribute__((noinline)) void col_major(const vector<double> &matrix, const vector<double> &vec,
                                         vector<double> &result, int n)
{
    for (int j = 0; j < n; j++)
    {
        result[j] = 0.0;
        for (int i = 0; i < n; i++)
        {
            result[j] += matrix[i * n + j] * vec[i]; // 列访问，跨越不同cache line
        }
    }
}

int main()
{
    // 测试不同规模，覆盖不同cache层级
    vector<int> sizes = {256, 512, 1024, 2048, 4096};
    int repeat_times = 100; // 重复次数

    cout << "=== 平凡算法（列优先访问）===" << endl;
    cout << "矩阵规模N, 总时间(ms), 单次时间(ms)" << endl;

    for (int n : sizes)
    {
        // 计算矩阵大小（字节）
        size_t matrix_size = n * n * sizeof(double);
        cout << "N=" << n << " 矩阵大小: " << matrix_size / 1024 << " KB" << endl;

        // 初始化数据
        vector<double> matrix(n * n);
        vector<double> vec(n);
        vector<double> result(n);

        // 用固定值初始化，便于验证正确性
        for (int i = 0; i < n; i++)
        {
            vec[i] = 1.0;
            for (int j = 0; j < n; j++)
            {
                matrix[i * n + j] = i + j;
            }
        }

        volatile double dummy = 0.0; // 防止优化

        auto start = high_resolution_clock::now();

        for (int t = 0; t < repeat_times; t++)
        {
            col_major(matrix, vec, result, n);
            dummy = result[0]; // 使用result防止优化
        }

        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start);
        double avg_ms = duration_ms.count() / (double)repeat_times;

        cout << n << ", " << duration_ms.count() << ", "
             << fixed << setprecision(3) << avg_ms << endl;
    }

    return 0;
}