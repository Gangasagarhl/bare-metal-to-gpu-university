// MP4 Listing 9: the GPU side of milestone 1. One source, two vendors:
//   AMD:    hipcc -std=c++17 -O2 --offload-arch=gfx90a --offload-arch=gfx942 --offload-arch=gfx1100 sgemm_mp4.hip -o sgemm_mp4
//   NVIDIA: nvcc -std=c++17 -O2 -arch=sm_80 -x cu -D__HIP_PLATFORM_NVIDIA__ sgemm_mp4.hip -o sgemm_mp4_nv
// It picks the tuning row for the device it finds, runs the MP3 shape matrix against the FP64
// reference, then times the chosen instance (warm-up 3, median of 20, device events) and prints
// one CSV record per size. The reference library is linked in milestone 3, not here.
#include <hip/hip_runtime.h>
#define LAUNCH(kernel, grid, block, ...) kernel<<<grid, block>>>(__VA_ARGS__)
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "launchers.hpp"
#include "mp4_check.hpp"
#include "shapes.hpp"

static void check(hipError_t e, const char* what)
{
    if (e != hipSuccess) {
        std::printf("%s failed: %s (%s)\n", what, hipGetErrorName(e), hipGetErrorString(e));
        std::exit(1);
    }
}

static void run(int idx, const float* A, const float* B, float* C, int M, int N, int K)
{
    if (!launchInstance(idx, A, B, C, M, N, K)) {
        std::printf("no instance %d\n", idx);
        std::exit(1);
    }
    check(hipGetLastError(), "launch");
}

struct DeviceBuffers
{
    float* A = nullptr;
    float* B = nullptr;
    float* C = nullptr;
    DeviceBuffers(std::size_t a, std::size_t b, std::size_t c)
    {
        check(hipMalloc(&A, a * sizeof(float)), "hipMalloc(A)");
        check(hipMalloc(&B, b * sizeof(float)), "hipMalloc(B)");
        check(hipMalloc(&C, c * sizeof(float)), "hipMalloc(C)");
    }
    ~DeviceBuffers()
    {
        (void)hipFree(A);
        (void)hipFree(B);
        (void)hipFree(C);
    }
    DeviceBuffers(const DeviceBuffers&) = delete;
    DeviceBuffers& operator=(const DeviceBuffers&) = delete;
};

int main()
{
    hipDeviceProp_t prop{};
    check(hipGetDeviceProperties(&prop, 0), "hipGetDeviceProperties");
    const std::string key = archKey(prop.gcnArchName, prop.major, prop.minor, kNvidiaPlatform);
    const TuningRow* row = findTuning(key);
    if (row == nullptr) {
        std::printf("device %s (key %s) has no tuning row: add one, with evidence\n", prop.name, key.c_str());
        return 2;
    }
    const InstanceInfo& inst = kInstances[row->instance];
    std::printf("platform %s, device %s, key %s, warpSize %d, instance %s\n", MP4_PLATFORM,
                prop.name, key.c_str(), prop.warpSize, inst.name);
    if (prop.warpSize != row->waveWidth || inst.threads % prop.warpSize != 0) {
        std::printf("tuning row expects wave %d; device reports %d: fix the row\n", row->waveWidth, prop.warpSize);
        return 2;
    }
    if (static_cast<std::size_t>(inst.ldsBytes) > prop.sharedMemPerBlock) {
        std::printf("instance needs %d bytes of LDS; device allows %zu per workgroup\n", inst.ldsBytes,
                    prop.sharedMemPerBlock);
        return 2;
    }

    int failed = 0;
    for (const Shape& s : testShapes()) {
        const std::vector<float> A = testMatrix(s.M, s.K, 1u);
        const std::vector<float> B = testMatrix(s.K, s.N, 2u);
        std::vector<float> C(static_cast<std::size_t>(s.M) * s.N);
        DeviceBuffers d(A.size(), B.size(), C.size());
        check(hipMemcpy(d.A, A.data(), A.size() * sizeof(float), hipMemcpyHostToDevice), "copy A");
        check(hipMemcpy(d.B, B.data(), B.size() * sizeof(float), hipMemcpyHostToDevice), "copy B");
        check(hipMemset(d.C, 0xFF, C.size() * sizeof(float)), "poison C");   // NaN if never written
        run(row->instance, d.A, d.B, d.C, s.M, s.N, s.K);
        check(hipMemcpy(C.data(), d.C, C.size() * sizeof(float), hipMemcpyDeviceToHost), "copy C");
        const Verdict v = checkGemm(A, B, C, s.M, s.N, s.K);
        std::printf("%4dx%4dx%4d  bad %d  worst err/bound %.3f\n", s.M, s.N, s.K, v.bad, v.worst);
        failed += v.bad > 0;
    }
    if (failed > 0) {
        std::printf("correctness FAILED on %d shape(s): no timing\n", failed);
        return 1;
    }

    std::printf("csv: platform,device,key,instance,M,N,K,median_ms,gflops\n");
    const int sizes[] = {1024, 2048, 4096};   // MP3 uses 4096 and two sizes of your choice
    for (int n : sizes) {
        DeviceBuffers d(static_cast<std::size_t>(n) * n, static_cast<std::size_t>(n) * n,
                        static_cast<std::size_t>(n) * n);
        const std::vector<float> A = testMatrix(n, n, 3u);   // the same kind of values as the tests, not zeros
        const std::vector<float> B = testMatrix(n, n, 4u);
        check(hipMemcpy(d.A, A.data(), A.size() * sizeof(float), hipMemcpyHostToDevice), "copy A");
        check(hipMemcpy(d.B, B.data(), B.size() * sizeof(float), hipMemcpyHostToDevice), "copy B");
        hipEvent_t t0, t1;
        check(hipEventCreate(&t0), "hipEventCreate");
        check(hipEventCreate(&t1), "hipEventCreate");
        for (int i = 0; i < 3; ++i) {
            run(row->instance, d.A, d.B, d.C, n, n, n);
        }
        std::vector<float> ms(20);
        for (float& m : ms) {
            check(hipEventRecord(t0, nullptr), "record t0");
            run(row->instance, d.A, d.B, d.C, n, n, n);
            check(hipEventRecord(t1, nullptr), "record t1");
            check(hipEventSynchronize(t1), "sync t1");
            check(hipEventElapsedTime(&m, t0, t1), "elapsed");
        }
        std::sort(ms.begin(), ms.end());
        const double med = (ms[9] + ms[10]) / 2.0;
        std::printf("csv: %s,%s,%s,%s,%d,%d,%d,%.4f,%.1f\n", MP4_PLATFORM, prop.name, key.c_str(), inst.name,
                    n, n, n, med, 2.0 * n * n * n / (med * 1e-3) * 1e-9);
        check(hipEventDestroy(t0), "destroy");
        check(hipEventDestroy(t1), "destroy");
    }
    return 0;
}
