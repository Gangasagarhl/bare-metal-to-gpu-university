// order.cpp - the same numbers added in two orders give two different float sums.
#include <cstddef>
#include <cstdio>
#include <vector>

int main()
{
    std::size_t const n = 1000000;
    std::vector<float> x(n);
    for (std::size_t i = 0; i < n; ++i) {
        x[i] = 1.0f / static_cast<float>(i + 1);
    }
    float inOrder = 0.0f;  // what the plain loop computes
    for (float v : x) {
        inOrder += v;
    }
    float lane[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // what an 8-lane SIMD sum computes
    for (std::size_t i = 0; i < n; ++i) {
        lane[i % 8] += x[i];
    }
    float eightLanes = 0.0f;
    for (float v : lane) {
        eightLanes += v;
    }
    double reference = 0.0;  // the same floats added in double precision
    for (float v : x) {
        reference += static_cast<double>(v);
    }
    std::printf("in order (1 accumulator):   %.7f\n", static_cast<double>(inOrder));
    std::printf("8 lanes, then combined:     %.7f\n", static_cast<double>(eightLanes));
    std::printf("double-precision reference: %.7f\n", reference);
    std::printf("equal? %s\n", inOrder == eightLanes ? "yes" : "no");
    return 0;
}
