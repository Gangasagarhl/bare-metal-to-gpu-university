// main.cpp - ordinary C++: no HIP header, built like any host file.
#include "saxpy.h"
#include <cstdio>
#include <vector>

int main()
{
    const int n = 100003;
    std::vector<float> x(n);
    std::vector<float> y(n, 1.0f);
    for (int i = 0; i < n; ++i) {
        x[i] = static_cast<float>(i % 512);
    }
    const char* err = saxpyOnGpu(2.0f, x, y);
    if (err[0] != '\0') {
        std::printf("GPU part failed: %s\n", err);
        return 1;
    }
    int mismatches = 0;
    for (int i = 0; i < n; ++i) {
        if (y[i] != 2.0f * x[i] + 1.0f) {
            ++mismatches;
        }
    }
    std::printf("saxpy n=%d: %d mismatches\n", n, mismatches);
    return mismatches == 0 ? 0 : 1;
}
