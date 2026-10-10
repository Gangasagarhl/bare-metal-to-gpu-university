// F6-30 Listing 3: why a graph helps short kernels and hardly helps long ones.
// A model with INVENTED TG-1 costs (not measurements of any GPU):
//   individual launches: the host needs hostUs per launch; the GPU needs gapUs
//                        between one kernel's end and the next kernel's start;
//   one graph launch:    the host needs graphUs once; the GPU needs nodeGapUs
//                        between nodes.
// Input lines: label kernels kernelUs hostUs gapUs graphUs nodeGapUs
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string label;
    int kernels = 0;
    double kUs = 0, hostUs = 0, gapUs = 0, graphUs = 0, nodeGapUs = 0;
    std::printf("%-14s %7s %9s %13s %11s %9s %16s\n", "case", "kernels", "kernel us",
                "individual us", "graph us", "speed-up", "saving/kernel us");
    while (std::cin >> label >> kernels >> kUs >> hostUs >> gapUs >> graphUs >> nodeGapUs) {
        double end = 0.0;                                   // individual launches
        for (int i = 0; i < kernels; ++i) {
            const double issued = (i + 1) * hostUs;         // host finishes launch call i
            const double start = std::max(issued, i == 0 ? 0.0 : end + gapUs);
            end = start + kUs;
        }
        const double individual = end;
        end = 0.0;                                          // one graph
        for (int i = 0; i < kernels; ++i) {
            const double start = i == 0 ? graphUs : end + nodeGapUs;
            end = start + kUs;
        }
        const double graph = end;
        std::printf("%-14s %7d %9.1f %13.1f %11.1f %8.2fx %16.2f\n", label.c_str(), kernels, kUs,
                    individual, graph, individual / graph, (individual - graph) / kernels);
    }
    return 0;
}
