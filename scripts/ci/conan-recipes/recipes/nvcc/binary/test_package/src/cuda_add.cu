#include "cuda_add.hpp"
#include <cuda_runtime.h>

__global__ void vector_add(const int *a, const int *b, int *result, int N) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < N) {
        result[idx] = a[idx] + b[idx];
    }
}

void launch_cuda_add(const int *a, const int *b, int *result, int N) {
    int *d_a = nullptr;
    int *d_b = nullptr;
    int *d_result = nullptr;

    cudaMalloc(&d_a, N * sizeof(int));
    cudaMalloc(&d_b, N * sizeof(int));
    cudaMalloc(&d_result, N * sizeof(int));

    cudaMemcpy(d_a, a, N * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, N * sizeof(int), cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocks = (N + threadsPerBlock - 1) / threadsPerBlock;
    vector_add<<<blocks, threadsPerBlock>>>(d_a, d_b, d_result, N);

    cudaMemcpy(result, d_result, N * sizeof(int), cudaMemcpyDeviceToHost);

    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_result);
}
