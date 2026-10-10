// F7-11 Listing 2: transpose a matrix through a 64 x 64 tile in the LDS.
// Workgroup = 64 x 4 work-items; each work-item moves 16 elements.
// transposeTile<64> uses tile[64][64] (naive), transposeTile<65> uses tile[64][65] (padded),
// transposeSwizzle keeps tile[64][64] and stores column c of row r at c ^ r (XOR swizzle).
// Complete program; the build container has no AMD GPU, so the run stops at the first call.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <hip/hip_runtime.h>

constexpr int TILE = 64;
constexpr int ROWS = 4;   // workgroup is TILE x ROWS work-items

template <int PITCH>
__global__ void __launch_bounds__(TILE * ROWS) transposeTile(const float* in, float* out, int n)
{
    __shared__ float tile[TILE][PITCH];
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int r = 0; r < TILE; r += ROWS) {          // read rows: neighbours read neighbours
        tile[threadIdx.y + r][threadIdx.x] = in[(y + r) * n + x];
    }
    __syncthreads();
    x = blockIdx.y * TILE + threadIdx.x;
    y = blockIdx.x * TILE + threadIdx.y;
    for (int r = 0; r < TILE; r += ROWS) {          // read a column of the tile
        out[(y + r) * n + x] = tile[threadIdx.x][threadIdx.y + r];
    }
}

__global__ void __launch_bounds__(TILE * ROWS) transposeSwizzle(const float* in, float* out, int n)
{
    __shared__ float tile[TILE][TILE];
    int x = blockIdx.x * TILE + threadIdx.x;
    int y = blockIdx.y * TILE + threadIdx.y;
    for (int r = 0; r < TILE; r += ROWS) {
        int row = threadIdx.y + r;
        tile[row][threadIdx.x ^ row] = in[(y + r) * n + x];   // element (row, c) lives at c ^ row
    }
    __syncthreads();
    x = blockIdx.y * TILE + threadIdx.x;
    y = blockIdx.x * TILE + threadIdx.y;
    for (int r = 0; r < TILE; r += ROWS) {
        int col = threadIdx.y + r;
        out[(y + r) * n + x] = tile[threadIdx.x][col ^ threadIdx.x];
    }
}

static void check(hipError_t err, const char* what)
{
    if (err != hipSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, hipGetErrorName(err), hipGetErrorString(err));
        std::exit(EXIT_FAILURE);
    }
}

int main()
{
    const int n = 1024;                               // a multiple of TILE in this listing
    std::vector<float> hIn(n * n), hOut(n * n);
    for (int i = 0; i < n * n; ++i) {
        hIn[i] = static_cast<float>(i);
    }
    float *dIn = nullptr, *dOut = nullptr;
    check(hipMalloc(&dIn, hIn.size() * sizeof(float)), "hipMalloc in");
    check(hipMalloc(&dOut, hOut.size() * sizeof(float)), "hipMalloc out");
    check(hipMemcpy(dIn, hIn.data(), hIn.size() * sizeof(float), hipMemcpyHostToDevice), "copy in");
    dim3 grid(n / TILE, n / TILE), block(TILE, ROWS);
    int errors = 0;
    for (int version = 0; version < 3; ++version) {
        check(hipMemset(dOut, 0, hOut.size() * sizeof(float)), "hipMemset out");
        if (version == 0) {
            transposeTile<TILE><<<grid, block>>>(dIn, dOut, n);        // naive: pitch 64
        } else if (version == 1) {
            transposeTile<TILE + 1><<<grid, block>>>(dIn, dOut, n);    // padded: pitch 65
        } else {
            transposeSwizzle<<<grid, block>>>(dIn, dOut, n);           // swizzled: pitch 64
        }
        check(hipGetLastError(), "kernel launch");
        check(hipMemcpy(hOut.data(), dOut, hOut.size() * sizeof(float), hipMemcpyDeviceToHost), "copy out");
        for (int r = 0; r < n; ++r) {
            for (int c = 0; c < n; ++c) {
                errors += hOut[c * n + r] != hIn[r * n + c];
            }
        }
    }
    std::printf("checked 3 versions of %d elements, %d errors\n", n * n, errors);
    check(hipFree(dIn), "hipFree in");
    check(hipFree(dOut), "hipFree out");
    return errors == 0 ? 0 : 1;
}
