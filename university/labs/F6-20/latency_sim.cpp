// F6-20 Listing 3: a cycle-by-cycle model of ONE warp scheduler running scaleIlp<ILP>.
// Each warp repeats: ILP loads, ILP multiplies (each waits for its load), ILP stores.
// The scheduler issues at most one instruction per cycle, from the first ready warp.
// The memory system accepts one new load every `loadGap` cycles (a bandwidth limit).
// Latencies and the gap are MODEL INPUTS read from stdin, not measurements of any GPU.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Warp
{
    int pc = 0;                    // position inside one round: 0 .. 3*ILP-1
    int roundsLeft = 0;
    std::vector<long> loadReady;   // cycle at which each of the ILP loaded values arrives
    long aluReady = 0;             // cycle at which the last multiply's result is ready
};

struct Result
{
    long cycles;
    double issueBusy;
    double avgLoadsInFlight;
};

Result simulate(int warps, int ilp, int elements, int memLatency, int aluLatency, int loadGap)
{
    std::vector<Warp> w(static_cast<size_t>(warps));
    for (Warp& x : w) {
        x.roundsLeft = elements / (warps * ilp);
        x.loadReady.assign(static_cast<size_t>(ilp), 0);
    }
    long cycle = 0, issued = 0, inFlightSum = 0, memoryFreeAt = 0;
    int done = 0;
    while (done < warps) {
        for (Warp& x : w) {                      // the first warp that can issue does so
            if (x.roundsLeft == 0) {
                continue;
            }
            const int step = x.pc / ilp;         // 0 = load, 1 = multiply, 2 = store
            const int i = x.pc % ilp;
            bool ready = true;
            if (step == 0) {
                ready = memoryFreeAt <= cycle;   // the memory system takes a new load
            } else if (step == 1) {
                ready = x.loadReady[static_cast<size_t>(i)] <= cycle;
            } else if (step == 2) {
                ready = x.aluReady <= cycle;
            }
            if (!ready) {
                continue;
            }
            if (step == 0) {
                x.loadReady[static_cast<size_t>(i)] = cycle + memLatency;
                memoryFreeAt = cycle + loadGap;
            } else if (step == 1) {
                x.aluReady = cycle + aluLatency;
            }
            ++issued;
            if (++x.pc == 3 * ilp) {
                x.pc = 0;
                if (--x.roundsLeft == 0) {
                    ++done;
                }
            }
            break;
        }
        for (const Warp& x : w) {                // loads issued but not yet arrived
            for (long r : x.loadReady) {
                inFlightSum += r > cycle ? 1 : 0;
            }
        }
        ++cycle;
    }
    return {cycle, double(issued) / double(cycle), double(inFlightSum) / double(cycle)};
}

int main()
{
    int memLatency = 0, aluLatency = 0, loadGap = 0, elements = 0;
    std::string label;
    std::cin >> memLatency >> aluLatency >> loadGap >> elements;
    std::printf("model inputs: load latency %d cycles, multiply latency %d cycles, one load per %d "
                "cycles at most, %d elements\n", memLatency, aluLatency, loadGap, elements);
    std::printf("memory roof of the model: %.2f elements per 100 cycles\n", 100.0 / loadGap);
    std::printf("%-22s %5s %4s %9s %13s %11s %15s\n", "case", "warps", "ILP", "cycles",
                "elem/100 cyc", "issue busy", "loads in flight");
    int warps = 0, ilp = 0;
    while (std::cin >> label >> warps >> ilp) {
        const Result r = simulate(warps, ilp, elements, memLatency, aluLatency, loadGap);
        std::printf("%-22s %5d %4d %9ld %13.2f %10.1f%% %15.1f\n", label.c_str(), warps, ilp,
                    r.cycles, 100.0 * elements / double(r.cycles), 100.0 * r.issueBusy,
                    r.avgLoadsInFlight);
    }
    return 0;
}
