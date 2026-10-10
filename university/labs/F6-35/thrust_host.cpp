// F6-35 Listing 2: Thrust algorithms, run on the CPU with the thrust::host execution policy.
// The same calls with thrust::device_vector (Listing 3) run on the GPU; here the library's
// sequential host back end does the work, so the results can be checked in this build.
#include <thrust/execution_policy.h>
#include <thrust/functional.h>
#include <thrust/reduce.h>
#include <thrust/scan.h>
#include <thrust/sequence.h>
#include <thrust/sort.h>
#include <thrust/transform.h>
#include <thrust/transform_reduce.h>
#include <thrust/version.h>
#include <cstdio>
#include <vector>

struct Saxpy
{
    float a;
    float operator()(float x, float y) const { return a * x + y; }
};

struct Square
{
    float operator()(float x) const { return x * x; }
};

int main()
{
    std::printf("Thrust %d.%d.%d (THRUST_VERSION %d)\n", THRUST_MAJOR_VERSION, THRUST_MINOR_VERSION,
                THRUST_SUBMINOR_VERSION, THRUST_VERSION);
    const int n = 8;
    std::vector<float> x(n);
    std::vector<float> y(n, 1.0f);
    thrust::sequence(thrust::host, x.begin(), x.end(), 0.0f);           // 0, 1, 2, ...
    thrust::transform(thrust::host, x.begin(), x.end(), y.begin(), y.begin(), Saxpy{2.0f});
    std::printf("saxpy y = 2x + 1:");
    for (float v : y) {
        std::printf(" %g", static_cast<double>(v));
    }
    const float sumSq = thrust::transform_reduce(thrust::host, y.begin(), y.end(), Square{}, 0.0f,
                                                 thrust::plus<float>());
    std::printf("\nsum of squares (one fused pass): %g\n", static_cast<double>(sumSq));
    std::vector<int> keys = {3, 1, 2, 1, 3, 2, 1, 0};
    std::vector<char> vals = {'d', 'a', 'c', 'b', 'e', 'f', 'g', 'h'};
    thrust::stable_sort_by_key(thrust::host, keys.begin(), keys.end(), vals.begin());
    std::printf("sort_by_key:");
    for (int i = 0; i < n; ++i) {
        std::printf(" %d%c", keys[static_cast<std::size_t>(i)], vals[static_cast<std::size_t>(i)]);
    }
    std::vector<int> counts = {2, 0, 3, 1, 4};
    std::vector<int> offsets(counts.size());
    thrust::exclusive_scan(thrust::host, counts.begin(), counts.end(), offsets.begin());
    std::printf("\nexclusive_scan of counts 2 0 3 1 4:");
    for (int o : offsets) {
        std::printf(" %d", o);
    }
    const int total = thrust::reduce(thrust::host, counts.begin(), counts.end());
    std::printf("\nreduce: %d\n", total);
    return 0;
}
