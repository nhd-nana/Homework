# 《并行程序设计》Lab1：CPU架构编程与性能优化

## 实验内容

1. 矩阵与向量内积（Cache优化）
2. n个数求和（超标量优化）

## 文件说明

| 文件                 | 说明                   |
| -------------------- | ---------------------- |
| matrix_col_major.cpp | 平凡算法：列优先访问   |
| matrix_row_major.cpp | 优化算法：行优先访问   |
| chain_sum.cpp        | 平凡算法：链式累加     |
| two_way_sum.cpp      | 优化算法：两路链式累加 |

## 编译运行

```bash
# 编译
g++ -g -O2 -o matrix_col_major.exe matrix_col_major.cpp
g++ -g -O2 -o matrix_row_major.exe matrix_row_major.cpp
g++ -g -O2 -o chain_sum.exe chain_sum.cpp
g++ -g -O2 -o two_way_sum.exe two_way_sum.cpp

# 运行
./matrix_col_major.exe
./matrix_row_major.exe
./chain_sum.exe
./two_way_sum.exe
```
