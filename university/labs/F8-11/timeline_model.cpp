// F8-11 Listing 3: the timeline of one data-parallel training step on two nodes, serial
// versus overlapped, in the alpha-beta model.
// Backward compute runs layer by layer (last layer first) on the compute stream. Each
// layer's gradient bucket is all-reduced on ONE communication channel, in the order the
// buckets become ready (first in, first out).
//   serial:  all communication starts after the whole backward pass;
//   overlap: bucket k starts when its layer is done AND the channel is free.
// Input: label; comm_alpha_us; comm_GBps (effective all-reduce bandwidth: time = alpha +
// MiB / GBps); ms_per_char for the drawing; layer count; then per layer, in backward
// order: layer id, backward ms, gradient MiB. The values in timeline_model.in are invented.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Layer
{
    int id;
    double computeMs;
    double mib;
};

struct Span
{
    double begin;
    double end;
    int id;
};

void drawRow(const char* name, const std::vector<Span>& spans, double msPerChar, double total)
{
    const int width = static_cast<int>(total / msPerChar + 0.5) + 1;
    std::string row(width, '.');
    for (const Span& s : spans) {
        const int b = static_cast<int>(s.begin / msPerChar + 0.5);
        const int e = std::max(b + 1, static_cast<int>(s.end / msPerChar + 0.5));
        for (int c = b; c < e && c < width; ++c) {
            row[c] = static_cast<char>('0' + s.id % 10);
        }
    }
    std::printf("  %-8s|%s|\n", name, row.c_str());
}

void schedule(const char* title, bool overlap, const std::vector<Layer>& layers, double alphaUs,
              double gbps, double msPerChar)
{
    std::vector<Span> compute;
    std::vector<Span> comm;
    double t = 0.0;
    for (const Layer& l : layers) {
        compute.push_back({t, t + l.computeMs, l.id});
        t += l.computeMs;
    }
    const double backwardEnd = t;
    double channelFree = overlap ? 0.0 : backwardEnd;
    double commTotal = 0.0;
    for (std::size_t k = 0; k < layers.size(); ++k) {
        const double ready = overlap ? compute[k].end : backwardEnd;
        const double begin = std::max(ready, channelFree);
        const double dur = alphaUs / 1e3 + layers[k].mib * 1.048576 / gbps;   // MiB -> ms
        comm.push_back({begin, begin + dur, layers[k].id});
        channelFree = begin + dur;
        commTotal += dur;
    }
    const double step = std::max(backwardEnd, channelFree);
    std::printf("%s\n", title);
    drawRow("compute", compute, msPerChar, step);
    drawRow("comm", comm, msPerChar, step);
    std::printf("  backward %.2f ms, communication %.2f ms, step %.2f ms, exposed communication "
                "%.2f ms (%.0f %% hidden)\n",
                backwardEnd, commTotal, step, step - backwardEnd,
                100.0 * (commTotal - (step - backwardEnd)) / commTotal);
}

}  // namespace

int main()
{
    std::string label;
    std::string name;
    double alphaUs = 0.0;
    double gbps = 0.0;
    double msPerChar = 0.0;
    int n = 0;
    std::cin >> label >> name >> alphaUs >> name >> gbps >> name >> msPerChar >> n;
    std::vector<Layer> layers(n);
    for (Layer& l : layers) {
        std::cin >> l.id >> l.computeMs >> l.mib;
    }
    std::printf("model input: %s; all-reduce time = %.0f us + MiB / (%.1f GB/s); "
                "one character = %.2f ms; digits are layer ids\n",
                label.c_str(), alphaUs, gbps, msPerChar);
    schedule("serial (communicate after the backward pass):", false, layers, alphaUs, gbps,
             msPerChar);
    schedule("overlap (start each bucket when its layer is done):", true, layers, alphaUs, gbps,
             msPerChar);
    return 0;
}
