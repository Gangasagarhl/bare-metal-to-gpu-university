// F6-35 Listing 3: the same work with thrust::device_vector, on the GPU.
// Thrust reports failures by throwing exceptions. A failed GPU allocation arrived in this build
// as a std::bad_alloc-derived exception, not as thrust::system_error, so both are caught.
// Untested on hardware: the build container has no GPU.
#include <thrust/device_vector.h>
#include <thrust/functional.h>
#include <thrust/host_vector.h>
#include <thrust/sequence.h>
#include <thrust/system_error.h>
#include <thrust/transform.h>
#include <thrust/transform_reduce.h>
#include <cstdio>
#include <cstdlib>
#include <new>

struct Saxpy
{
    float a;
    __host__ __device__ float operator()(float x, float y) const { return a * x + y; }
};

struct Square
{
    __host__ __device__ float operator()(float x) const { return x * x; }
};

int main()
{
    try {
        const int n = 1 << 20;
        thrust::device_vector<float> x(n);                 // allocates GPU memory
        thrust::device_vector<float> y(n, 1.0f);
        thrust::sequence(x.begin(), x.end(), 0.0f);
        thrust::transform(x.begin(), x.end(), y.begin(), y.begin(), Saxpy{2.0f});
        const float sumSq = thrust::transform_reduce(y.begin(), y.end(), Square{}, 0.0f, thrust::plus<float>());
        thrust::host_vector<float> h(y.begin(), y.begin() + 4);   // copy back four values
        std::printf("y[0..3] = %g %g %g %g, sum of squares %g\n", static_cast<double>(h[0]),
                    static_cast<double>(h[1]), static_cast<double>(h[2]), static_cast<double>(h[3]),
                    static_cast<double>(sumSq));
    } catch (const thrust::system_error& e) {
        std::fprintf(stderr, "Thrust error: %s\n", e.what());
        return EXIT_FAILURE;
    } catch (const std::bad_alloc& e) {
        std::fprintf(stderr, "allocation failed: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}
