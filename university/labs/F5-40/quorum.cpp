// quorum.cpp - quorum reads and writes on N = 3 replicas (F5-40, Listing 2).
// Part 1: a writer updates one key; each replica receives each version after a random delay.
// A write is acknowledged after W replicas have it; a read asks R replicas and returns the
// newest version among them. Counts reads that miss the newest acknowledged version (stale),
// with the mean write and read waiting time, for several (R, W).
// Part 2: a scripted sloppy quorum: R + W > N, yet the read misses the write.
// Delays are exercise values in abstract "ticks", not measurements of any network.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

constexpr int kN = 3;
constexpr int kWrites = 20000;

struct Rng
{
    std::mt19937_64 g{2026};
    double uniform() { return static_cast<double>(g() >> 11) * 0x1.0p-53; }
    int below(int n) { return static_cast<int>(uniform() * n); }
    // most messages are fast; some are slow (a busy replica, a long queue)
    int delay() { return uniform() < 0.8 ? 1 + below(5) : 20 + below(180); }
};

void part1()
{
    std::printf("== part 1: N=%d, %d writes, one read after each acknowledged write\n", kN, kWrites);
    std::printf("   R W  R+W>N  stale reads   mean write wait  mean read wait\n");
    const int cases[][2] = {{1, 1}, {1, 2}, {2, 1}, {2, 2}, {1, 3}, {3, 1}, {3, 3}};
    for (const auto& rw : cases) {
        const int R = rw[0];
        const int W = rw[1];
        Rng rng;   // same random delays for every (R, W)
        // arrive[v][i]: time version v reaches replica i
        std::vector<std::vector<int>> arrive(kWrites, std::vector<int>(kN));
        int t = 0;
        long writeWait = 0;
        long readWait = 0;
        int stale = 0;
        for (int v = 0; v < kWrites; ++v) {
            std::vector<int> d(kN);
            for (int i = 0; i < kN; ++i) {
                d[i] = rng.delay();
                arrive[v][i] = t + d[i];
            }
            std::vector<int> sorted = d;
            std::sort(sorted.begin(), sorted.end());
            const int ack = t + sorted[W - 1];   // acknowledged when W replicas have it
            writeWait += sorted[W - 1];
            // a read starts right after the acknowledgement and asks R distinct random replicas
            std::vector<int> ids = {0, 1, 2};
            std::shuffle(ids.begin(), ids.end(), rng.g);
            std::vector<int> replyDelay;
            int newest = -1;
            for (int k = 0; k < R; ++k) {
                const int rep = ids[k];
                // the replica holds the newest version that has arrived by the read time
                for (int u = v; u >= 0 && u >= v - 400; --u) {
                    if (arrive[u][rep] <= ack) {
                        newest = std::max(newest, u);
                        break;
                    }
                }
                replyDelay.push_back(rng.delay());
            }
            std::sort(replyDelay.begin(), replyDelay.end());
            readWait += replyDelay[R - 1];
            stale += newest < v;
            t = ack + 1;   // the writer sends the next version after the acknowledgement
        }
        std::printf("   %d %d  %-5s  %5d (%5.2f %%)  %8.1f ticks   %8.1f ticks\n", R, W,
                    R + W > kN ? "yes" : "no", stale, 100.0 * stale / kWrites,
                    static_cast<double>(writeWait) / kWrites, static_cast<double>(readWait) / kWrites);
    }
}

void part2()
{
    std::printf("== part 2: sloppy quorum, N=3, W=2, R=2 (R+W>N)\n");
    std::printf("   home replicas of key k: A, B, C; the next node on the ring is D\n");
    std::printf("   t=1  writer can reach A and D only (B, C unreachable from the writer)\n");
    std::printf("   t=1  write k=v2 -> A: stored v2\n");
    std::printf("   t=1  write k=v2 -> D: stored v2 with hint 'belongs to B' (hinted handoff)\n");
    std::printf("   t=1  W=2 acknowledgements (A, D): write ACKNOWLEDGED\n");
    std::printf("   t=2  reader can reach B and C only\n");
    const int bVersion = 1;
    const int cVersion = 1;
    std::printf("   t=2  read k <- B: v%d; read k <- C: v%d; R=2 replies: returns v%d\n", bVersion, cVersion,
                std::max(bVersion, cVersion));
    std::printf("   result: STALE read although R+W>N: the write quorum {A,D} and the read quorum {B,C}\n");
    std::printf("           do not intersect, because D is not one of the key's home replicas\n");
    std::printf("   t=9  partition heals; D hands v2 over to B and deletes its hinted copy\n");
}

int main()
{
    part1();
    part2();
    return 0;
}
