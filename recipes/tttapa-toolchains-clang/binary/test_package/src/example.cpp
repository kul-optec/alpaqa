#include <iostream>
#include <omp.h>

int main() {
    std::cout << "max #thr = " << omp_get_max_threads() << "\n";
#pragma omp parallel
    {
#pragma omp single
        std::cout << "    #thr = " << omp_get_num_threads() << "\n";
    }
}
