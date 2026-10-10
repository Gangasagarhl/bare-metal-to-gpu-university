// F6-21 Listing 1: the naive SGEMM kernel. C = A * B, all row-major FP32:
// A is M x K, B is K x N, C is M x N. One thread computes one element of C.
// Indices are int: correct while M*K, K*N and M*N stay below 2^31.
#pragma once

__global__ void sgemmNaive(const float* A, const float* B, float* C, int M, int N, int K)
{
    const int col = blockIdx.x * blockDim.x + threadIdx.x;   // neighbouring threads: neighbouring columns
    const int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (row < M && col < N) {
        float acc = 0.0f;
        for (int k = 0; k < K; ++k) {
            acc += A[row * K + k] * B[k * N + col];           // 2 loads for 1 multiply-add
        }
        C[row * N + col] = acc;
    }
}
