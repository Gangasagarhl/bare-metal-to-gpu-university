// F6-05 Listing 3: a CPU model of how launch errors are reported (the university's own
// teaching model of TG-1, the invented teaching GPU; it is not CUDA and not a GPU).
// It follows three rules written in the CUDA 12.0 header comments (chapter source H1):
//   1. a launch that asks for too many threads per block, or too much shared memory,
//      fails with cudaErrorInvalidConfiguration and the kernel does not run;
//   2. cudaGetLastError returns the last error and resets it; cudaPeekAtLastError does not;
//   3. an invalid memory access inside a kernel (cudaErrorIllegalAddress) is reported by a
//      later call, and every later call returns the same error.
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

enum class Err { Success, InvalidConfiguration, IllegalAddress };

const char* name(Err e)
{
    switch (e) {
    case Err::Success: return "cudaSuccess";
    case Err::InvalidConfiguration: return "cudaErrorInvalidConfiguration";
    case Err::IllegalAddress: return "cudaErrorIllegalAddress";
    }
    return "?";
}

class Tg1Model
{
public:
    static constexpr int kMaxThreadsPerBlock = 512;   // TG-1: 16 resident warps x 32 per SM
    static constexpr int kMaxSharedPerBlock = 32768;  // TG-1: 32 KiB shared memory per SM

    // Runs body(i) for every thread i of the grid, like a kernel; size is the length of
    // the array the kernel writes, used to detect an out-of-range access.
    Err launch(int blocks, int threads, int sharedBytes, int size,
               const std::function<void(int)>& body)
    {
        if (sticky_ != Err::Success) { return record(sticky_); }
        if (threads > kMaxThreadsPerBlock || threads < 1 || blocks < 1 ||
            sharedBytes > kMaxSharedPerBlock) {
            return record(Err::InvalidConfiguration);      // rule 1: nothing runs
        }
        for (int i = 0; i < blocks * threads; ++i) {
            if (i >= size) { pending_ = Err::IllegalAddress; break; }  // rule 3
            body(i);
        }
        return Err::Success;                               // the launch itself was accepted
    }
    Err synchronize()                                      // like cudaDeviceSynchronize
    {
        if (pending_ != Err::Success) { sticky_ = pending_; pending_ = Err::Success; }
        return record(sticky_);
    }
    Err copy() { return record(sticky_); }                 // any later runtime call
    Err getLastError() { Err e = last_; last_ = sticky_; return e; }  // rule 2: resets
    Err peekAtLastError() const { return last_; }          // rule 2: does not reset

private:
    Err record(Err e) { if (e != Err::Success) { last_ = e; } return e; }
    Err last_ = Err::Success;
    Err pending_ = Err::Success;
    Err sticky_ = Err::Success;
};

void kofi(bool checked)
{
    Tg1Model gpu;
    const int n = 1 << 16;
    std::vector<float> in(n), out(n, 0.0f);
    for (int i = 0; i < n; ++i) { in[i] = 0.5f * i; }
    const int threads = 2048;
    gpu.launch((n + threads - 1) / threads, threads, 0, n,
               [&](int i) { out[i] = in[i] * in[i]; });
    if (checked) {
        std::printf("  check after launch: %s\n", name(gpu.getLastError()));
    }
    double sum = 0.0;
    for (int i = 0; i < n; ++i) { sum += out[i]; }
    std::printf("  out[1] = %.2f, out[2] = %.2f, sum of all outputs = %.2f\n", out[1], out[2], sum);
}

int main()
{
    std::printf("1. Kofi's program on the TG-1 model, no checks:\n");
    kofi(false);
    std::printf("2. The same program with a check after the launch:\n");
    kofi(true);

    std::printf("3. Peek versus get:\n");
    Tg1Model gpu;
    gpu.launch(1, 1024, 0, 1024, [](int) {});
    std::printf("  peek: %s\n", name(gpu.peekAtLastError()));
    std::printf("  peek: %s\n", name(gpu.peekAtLastError()));
    std::printf("  get:  %s\n", name(gpu.getLastError()));
    std::printf("  get:  %s\n", name(gpu.getLastError()));

    std::printf("4. A kernel without its bounds check (1024 threads, 1000 elements):\n");
    Tg1Model gpu2;
    std::vector<int> v(1000, 0);
    Err e = gpu2.launch(4, 256, 0, 1000, [&](int i) { v[i] = 7; });
    std::printf("  launch:            %s\n", name(e));
    std::printf("  getLastError:      %s\n", name(gpu2.getLastError()));
    std::printf("  synchronize:       %s\n", name(gpu2.synchronize()));
    std::printf("  next copy:         %s\n", name(gpu2.copy()));
    std::printf("  next launch:       %s\n", name(gpu2.launch(1, 32, 0, 32, [](int) {})));
    return 0;
}
