// BR-01 Listing 4: a second memory. Allocate on the device, copy there and back, compare.
// Still no kernel: this step tests only allocation, copies and error checking.
#include <cstddef>
#include <cstdio>
#include <exception>
#include <vector>
#include "br01_check.h"

int main()
{
    try {
        const std::size_t n = 1000003;
        std::vector<float> hA(n), hBack(n, -1.0f);
        for (std::size_t i = 0; i < n; ++i) {
            hA[i] = 1.0f * static_cast<float>(i);
        }
        DeviceBuffer<float> dA(n);                  // cudaMalloc, checked
        dA.copyFromHost(hA);                        // host -> device, checked
        dA.copyToHost(hBack);                       // device -> host, checked
        std::size_t mismatches = 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (hBack[i] != hA[i]) {
                ++mismatches;
            }
        }
        std::printf("round trip of %zu floats: %zu mismatches\n", n, mismatches);
        return mismatches == 0 ? 0 : 1;
    } catch (const std::exception& e) {             // any DeviceBuffer already built is freed
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
}
