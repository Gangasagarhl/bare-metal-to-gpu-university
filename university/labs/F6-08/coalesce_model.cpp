// F6-08 Listing 1: a model of how one warp's 32 loads become memory segments
// (the university's own teaching model for TG-1; the real rules are generation-specific).
// Thread t of the warp reads element  offset + t * stride  of an array whose start is
// aligned to the segment size. The model counts the distinct aligned segments touched.
// Input lines: label elementBytes stride offset segmentBytes
#include <cstdio>
#include <iostream>
#include <set>
#include <string>

int main()
{
    std::printf("%-26s %5s %6s %6s %7s %10s %9s %10s %10s\n", "case", "elem", "stride", "offset",
                "segment", "requested", "segments", "moved", "efficiency");
    std::string label;
    long long elem = 0, stride = 0, offset = 0, segment = 0;
    while (std::cin >> label >> elem >> stride >> offset >> segment) {
        if (elem <= 0 || stride < 0 || offset < 0 || segment <= 0) {
            std::printf("%-26s invalid input\n", label.c_str());
            continue;
        }
        std::set<long long> touched;
        for (long long t = 0; t < 32; ++t) {             // the 32 threads of one warp
            const long long first = (offset + t * stride) * elem;
            const long long last = first + elem - 1;     // an element may straddle two
            for (long long s = first / segment; s <= last / segment; ++s) { touched.insert(s); }
        }
        const long long requested = 32 * elem;
        const long long moved = static_cast<long long>(touched.size()) * segment;
        std::printf("%-26s %5lld %6lld %6lld %7lld %10lld %9zu %10lld %9.1f%%\n", label.c_str(),
                    elem, stride, offset, segment, requested, touched.size(), moved,
                    100.0 * static_cast<double>(requested) / static_cast<double>(moved));
    }
    return 0;
}
