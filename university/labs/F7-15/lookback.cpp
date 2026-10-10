// F7-15 Listing 4: a CPU model of a single-pass scan with decoupled look-back.
// Tiles run "concurrently": a scheduler gives one step at a time to a random live tile.
// Each tile publishes its status: X = nothing yet, A = own aggregate, P = inclusive prefix.
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <random>
#include <string>
#include <vector>

enum class Flag { X, A, P };
struct Status { Flag flag = Flag::X; long long value = 0; };

struct Stats { long long steps = 0; long long lookedAt = 0; long long waits = 0; };

std::vector<long long> chainedScan(const std::vector<long long>& in, int tile, std::mt19937& gen,
                                   Stats& st)
{
    const int n = static_cast<int>(in.size());
    const int tiles = (n + tile - 1) / tile;
    std::vector<long long> out(n);
    std::vector<Status> status(tiles);
    std::vector<int> phase(tiles, 0);        // 0 = reduce own tile, 1 = look back, 2 = done
    std::vector<int> cursor(tiles);          // which predecessor the look-back reads next
    std::vector<long long> exclusive(tiles, 0);
    std::vector<int> live(tiles);
    std::iota(live.begin(), live.end(), 0);
    while (!live.empty()) {
        std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
        const std::size_t k = pick(gen);
        const int t = live[k];
        ++st.steps;
        if (phase[t] == 0) {                 // reduce the tile, publish A (or P for tile 0)
            long long agg = 0;
            for (int i = t * tile; i < std::min(n, (t + 1) * tile); ++i) {
                agg += in[i];
            }
            status[t] = {t == 0 ? Flag::P : Flag::A, agg};
            cursor[t] = t - 1;
            phase[t] = (t == 0) ? 2 : 1;
        } else if (phase[t] == 1) {          // look at one predecessor per step
            const Status s = status[cursor[t]];
            ++st.lookedAt;
            if (s.flag == Flag::X) {
                ++st.waits;                  // predecessor not ready: try again later
            } else {
                exclusive[t] += s.value;
                if (s.flag == Flag::P) {
                    status[t] = {Flag::P, exclusive[t] + status[t].value};
                    phase[t] = 2;
                } else {
                    --cursor[t];
                }
            }
        }
        if (phase[t] == 2) {                 // write the tile's outputs and leave
            long long run = exclusive[t];
            for (int i = t * tile; i < std::min(n, (t + 1) * tile); ++i) {
                out[i] = run;
                run += in[i];
            }
            live.erase(live.begin() + static_cast<long>(k));
        }
    }
    return out;
}

int main()
{
    std::mt19937 gen(7);
    const int tile = 256;
    const int sizes[] = {1, 255, 256, 257, 65535, 65537, 1000003};
    const char* kinds[] = {"random", "sorted", "reverse", "all-equal"};
    int failures = 0;
    std::printf("%-9s %-8s %-6s %-8s %-10s %s\n", "input", "n", "tiles", "result", "looks/tile", "waits");
    for (const char* kind : kinds) {
        for (int n : sizes) {
            std::vector<long long> in(n);
            std::uniform_int_distribution<int> val(-1000, 1000);
            for (int i = 0; i < n; ++i) {
                in[i] = val(gen);
            }
            const std::string k = kind;
            if (k == "sorted") {
                std::sort(in.begin(), in.end());
            } else if (k == "reverse") {
                std::sort(in.rbegin(), in.rend());
            } else if (k == "all-equal") {
                std::fill(in.begin(), in.end(), 3);
            }
            std::vector<long long> ref(n);
            std::exclusive_scan(in.begin(), in.end(), ref.begin(), 0LL);
            Stats st;
            const std::vector<long long> got = chainedScan(in, tile, gen, st);
            const bool ok = (got == ref);
            failures += ok ? 0 : 1;
            const int tiles = (n + tile - 1) / tile;
            std::printf("%-9s %-8d %-6d %-8s %-10.2f %lld\n", kind, n, tiles, ok ? "ok" : "WRONG",
                        static_cast<double>(st.lookedAt) / tiles, st.waits);
        }
    }
    std::printf("failures: %d\n", failures);
    return failures == 0 ? 0 : 1;
}
