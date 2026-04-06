#include <omp.h>
#include <stdio.h>

int main() {
    printf("max #thr = %d\n", omp_get_max_threads());
#pragma omp parallel
    {
#pragma omp single
        printf("    #thr = %d\n", omp_get_num_threads());
    }
}
