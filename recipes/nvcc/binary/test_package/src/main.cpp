#include <iostream>
#include <numeric>
#include <vector>

#include "cuda_add.hpp"

int main() {
    const int N = 1 << 10;
    std::vector<int> a(N), b(N), result(N);

    std::iota(a.begin(), a.end(), 1);
    std::iota(b.begin(), b.end(), N);

    launch_cuda_add(a.data(), b.data(), result.data(), N);

    for (int i = 0; i < 10; ++i)
        std::cout << a[i] << " + " << b[i] << " = " << result[i] << '\t';
    std::cout << std::endl;
}
