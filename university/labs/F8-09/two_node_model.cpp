// F8-09 Listing 4: three ways to move a GPU buffer from a GPU in node A to a GPU in node B,
// in the alpha-beta model:
//   staged     copy device->host on A, send host->host over the network, copy host->device
//              on B, one after the other;
//   pipelined  the same three stages on chunks, so chunk k+1 is copied while chunk k is on
//              the network (what a staging MPI library or your own code can do);
//   GPUDirect  the NIC reads A's GPU memory and writes B's GPU memory: network stage only.
// Input: a label, six "name value" parameters, then message sizes in bytes.
// The parameters in two_node_model.in are invented teaching values, not measurements.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

namespace {

double stageUs(double alphaUs, double gbps, double bytes)
{
    return alphaUs + bytes / (gbps * 1e3);   // 1 GB/s = 1,000 bytes per microsecond
}

}  // namespace

int main()
{
    std::string label;
    std::cin >> label;
    std::map<std::string, double> p;
    for (int i = 0; i < 6; ++i) {
        std::string name;
        double value = 0.0;
        std::cin >> name >> value;
        p[name] = value;
    }
    std::printf("model input: %s\n", label.c_str());
    std::printf("copy: alpha %.1f us, %.1f GB/s | network: alpha %.1f us, %.1f GB/s | "
                "NIC reading GPU memory: %.1f GB/s | chunk %.0f bytes\n",
                p["copy_alpha_us"], p["copy_GBps"], p["net_alpha_us"], p["net_GBps"],
                p["gdr_GBps"], p["chunk_bytes"]);
    std::printf("%10s %12s %12s %12s %10s\n", "bytes", "staged us", "pipelined us",
                "GPUDirect us", "staged/GDR");
    double bytes = 0.0;
    while (std::cin >> bytes) {
        const double copy = stageUs(p["copy_alpha_us"], p["copy_GBps"], bytes);
        const double net = stageUs(p["net_alpha_us"], p["net_GBps"], bytes);
        const double staged = copy + net + copy;

        // Pipeline of three stages over k chunks: fill the pipe once, then one chunk per
        // slowest-stage time.
        const double chunk = std::min(bytes, p["chunk_bytes"]);
        const double k = std::ceil(bytes / chunk);
        const double c1 = stageUs(p["copy_alpha_us"], p["copy_GBps"], chunk);
        const double n1 = stageUs(p["net_alpha_us"], p["net_GBps"], chunk);
        const double pipelined = (c1 + n1 + c1) + (k - 1.0) * std::max(c1, n1);

        // GPUDirect RDMA: the network stage, limited by the slower of the network and the
        // NIC's access to GPU memory.
        const double gdr =
            stageUs(p["net_alpha_us"], std::min(p["net_GBps"], p["gdr_GBps"]), bytes);
        std::printf("%10.0f %12.2f %12.2f %12.2f %10.2f\n", bytes, staged, pipelined, gdr,
                    staged / gdr);
    }
    return 0;
}
