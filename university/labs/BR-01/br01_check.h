// BR-01 Listing 3: checking every runtime call, and an RAII owner for device memory.
// Host code is ordinary C++: it may throw exceptions and use destructors.
#pragma once
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#include <cuda_runtime.h>

inline void cudaCheck(cudaError_t err, const char* call, const char* file, int line)
{
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(file) + ":" + std::to_string(line) + ": " + call +
                                 " failed: " + cudaGetErrorName(err) + " (" +
                                 cudaGetErrorString(err) + ")");
    }
}

// Wrap every runtime call: CUDA_CHECK(cudaMalloc(&p, bytes));
#define CUDA_CHECK(call) cudaCheck((call), #call, __FILE__, __LINE__)

template <typename T>
class DeviceBuffer                                  // owns count elements of device memory
{
public:
    explicit DeviceBuffer(std::size_t count) : count_(count)
    {
        CUDA_CHECK(cudaMalloc(&ptr_, count_ * sizeof(T)));
    }
    ~DeviceBuffer()
    {
        if (cudaFree(ptr_) != cudaSuccess) {        // a destructor must not throw
            std::fprintf(stderr, "warning: cudaFree failed\n");
        }
    }
    DeviceBuffer(const DeviceBuffer&) = delete;     // one owner per allocation
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;

    T* get() const { return ptr_; }                 // a DEVICE pointer: only kernels use it

    void copyFromHost(const std::vector<T>& host)
    {
        requireSize(host.size());
        CUDA_CHECK(cudaMemcpy(ptr_, host.data(), count_ * sizeof(T), cudaMemcpyHostToDevice));
    }
    void copyToHost(std::vector<T>& host) const     // returns only once the copy has completed
    {
        requireSize(host.size());
        CUDA_CHECK(cudaMemcpy(host.data(), ptr_, count_ * sizeof(T), cudaMemcpyDeviceToHost));
    }

private:
    void requireSize(std::size_t n) const
    {
        if (n != count_) {
            throw std::length_error("DeviceBuffer: host and device sizes differ");
        }
    }
    T* ptr_ = nullptr;
    std::size_t count_ = 0;
};
