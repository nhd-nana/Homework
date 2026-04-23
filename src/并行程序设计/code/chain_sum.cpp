#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

using namespace std;
using namespace chrono;

// 平凡算法：链式累加（有数据依赖）
__attribute__((noinline)) double chain_sum(const vector<double> &arr, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        sum += arr[i]; // 每次加法依赖上一次结果
    }
    return sum;
}

int main()
{
    vector<int> sizes = {1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288};
    int repeat_times = 10000; // 重复次数

    cout << "=== 平凡算法（链式累加）===" << endl;
    cout << "元素个数N, 总时间(ms), 单次时间(us)" << endl;

    for (int n : sizes)
    {
        // 初始化数组
        vector<double> arr(n);
        for (int i = 0; i < n; i++)
        {
            arr[i] = 1.0; // 固定值，便于验证
        }

        volatile double result = 0.0; // volatile防止优化

        auto start = high_resolution_clock::now();

        for (int t = 0; t < repeat_times; t++)
        {
            result = chain_sum(arr, n);
        }

        auto end = high_resolution_clock::now();
        auto duration_ms = duration_cast<milliseconds>(end - start);
        double avg_us = duration_ms.count() * 1000.0 / repeat_times;

        cout << n << ", " << duration_ms.count() << ", "
             << fixed << setprecision(3) << avg_us << endl;
    }

    return 0;
}