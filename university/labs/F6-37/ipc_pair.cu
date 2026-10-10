// F6-37 Listing 2: sharing one GPU allocation between two processes with CUDA IPC.
// The parent forks first (before any CUDA call, so each process creates its own CUDA context),
// then allocates and fills a buffer, exports a handle and sends its bytes through a pipe.
// The child opens the handle, reads the buffer through its own pointer, and closes it.
// Untested on hardware: the build container has no GPU.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <cuda_runtime.h>

static pid_t gChild = 0;        // set in the parent: on failure, let the child finish first
static int gToChild = -1;

static void check(cudaError_t err, const char* what)
{
    if (err != cudaSuccess) {
        std::fprintf(stderr, "%s failed: %s (%s)\n", what, cudaGetErrorName(err), cudaGetErrorString(err));
        if (gChild > 0) {
            close(gToChild);           // the child sees end-of-file instead of a handle
            waitpid(gChild, nullptr, 0);
        }
        std::exit(EXIT_FAILURE);
    }
}

__global__ void fill(int* p, int n, int value)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        p[i] = value + i;
    }
}

constexpr int kN = 1024;

int child(int readFd, int ackFd)
{
    cudaIpcMemHandle_t handle;
    if (read(readFd, &handle, sizeof(handle)) != static_cast<ssize_t>(sizeof(handle))) {
        std::fprintf(stderr, "child: no handle received (the parent stopped first)\n");
        return EXIT_FAILURE;
    }
    close(readFd);
    check(cudaSetDevice(0), "child cudaSetDevice");
    void* p = nullptr;
    check(cudaIpcOpenMemHandle(&p, handle, cudaIpcMemLazyEnablePeerAccess), "cudaIpcOpenMemHandle");
    std::vector<int> h(kN);
    check(cudaMemcpy(h.data(), p, kN * sizeof(int), cudaMemcpyDeviceToHost), "child copy");
    check(cudaIpcCloseMemHandle(p), "cudaIpcCloseMemHandle");
    std::printf("child: read %d, %d, ..., %d through its own mapping\n", h[0], h[1], h[kN - 1]);
    const char ok = 'k';
    return write(ackFd, &ok, 1) == 1 ? EXIT_SUCCESS : EXIT_FAILURE;
}

int main()
{
    int toChild[2];
    int toParent[2];
    if (pipe(toChild) != 0 || pipe(toParent) != 0) {
        std::perror("pipe");
        return EXIT_FAILURE;
    }
    const pid_t pid = fork();                           // fork before the first CUDA call
    if (pid < 0) {
        std::perror("fork");
        return EXIT_FAILURE;
    }
    if (pid == 0) {
        close(toChild[1]);                              // keep only the ends this process uses
        close(toParent[0]);
        std::exit(child(toChild[0], toParent[1]));
    }
    gChild = pid;
    gToChild = toChild[1];
    close(toChild[0]);
    close(toParent[1]);
    int* d = nullptr;
    check(cudaSetDevice(0), "parent cudaSetDevice");
    check(cudaMalloc(&d, kN * sizeof(int)), "cudaMalloc");
    fill<<<(kN + 255) / 256, 256>>>(d, kN, 1000);
    check(cudaGetLastError(), "fill launch");
    check(cudaDeviceSynchronize(), "fill");             // data complete before anyone else reads it
    cudaIpcMemHandle_t handle;
    check(cudaIpcGetMemHandle(&handle, d), "cudaIpcGetMemHandle");
    if (write(toChild[1], &handle, sizeof(handle)) != static_cast<ssize_t>(sizeof(handle))) {
        std::perror("write");
    }
    char ack = 0;
    const bool acked = read(toParent[0], &ack, 1) == 1;  // keep the memory alive until the child is done
    int status = 0;
    waitpid(pid, &status, 0);
    check(cudaFree(d), "cudaFree");
    std::printf("parent: child %s, exit status %d\n", acked ? "acknowledged" : "did not acknowledge",
                WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    return acked ? EXIT_SUCCESS : EXIT_FAILURE;
}
