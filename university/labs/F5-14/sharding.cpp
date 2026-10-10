// sharding.cpp - three ways to split keys over machines, and what each costs.
//   1. hash mod N        : shard = hash(key) % N
//   2. consistent hashing: nodes and keys are hashed onto a ring; a key belongs to the first
//                          node point clockwise; each node owns V points (virtual nodes)
//   3. key ranges        : shard i holds keys in [lower_i, lower_{i+1})
// The hash is 64-bit FNV-1a, defined by its arithmetic, so every machine gets the same result.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

std::uint64_t fnv1a(const std::string& s)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    h ^= h >> 33;             // final mixing step: FNV's low bits are weak for similar keys
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h;
}

struct Ring
{
    std::vector<std::pair<std::uint64_t, int>> points;   // (position, node), sorted
    Ring(int nodes, int virtuals)
    {
        for (int n = 0; n < nodes; ++n) {
            add(n, virtuals);
        }
    }
    void add(int node, int virtuals)
    {
        for (int v = 0; v < virtuals; ++v) {
            const std::string name = "node-" + std::to_string(node) + "#" + std::to_string(v);
            points.push_back({fnv1a(name), node});
        }
        std::sort(points.begin(), points.end());
    }
    int owner(std::uint64_t h) const
    {
        auto it = std::lower_bound(points.begin(), points.end(), std::make_pair(h, -1));
        return it == points.end() ? points.front().second : it->second;   // wrap around
    }
};

double imbalance(const std::vector<int>& load)   // most loaded node / average
{
    double sum = 0;
    for (int x : load) {
        sum += x;
    }
    return *std::max_element(load.begin(), load.end()) / (sum / load.size());
}

int main()
{
    const int keys = 100000;
    std::vector<std::uint64_t> h(keys);
    for (int k = 0; k < keys; ++k) {
        h[k] = fnv1a("user:" + std::to_string(k));
    }

    std::printf("1. hash mod N, %d keys\n", keys);
    for (int n : {4, 10}) {
        int moved = 0;
        for (std::uint64_t x : h) {
            moved += (x % n != x % (n + 1)) ? 1 : 0;
        }
        std::printf("   N %2d -> %2d: %5.1f %% of keys move (only %4.1f %% would have to)\n", n,
                    n + 1, 100.0 * moved / keys, 100.0 / (n + 1));
    }

    std::printf("\n2. consistent hashing, 4 nodes -> add a 5th (ideal: 20.0 %% move, "
                "imbalance 1.00)\n");
    for (int v : {1, 10, 100, 1000}) {
        Ring before(4, v);
        Ring after(4, v);
        after.add(4, v);
        int moved = 0;
        bool onlyToNew = true;
        std::vector<int> load(5, 0);
        for (std::uint64_t x : h) {
            const int a = before.owner(x);
            const int b = after.owner(x);
            if (a != b) {
                ++moved;
                onlyToNew = onlyToNew && b == 4;
            }
            ++load[b];
        }
        std::printf("   V %4d virtual nodes each: %5.1f %% move, all to the new node: %s, "
                    "imbalance %.2f\n", v, 100.0 * moved / keys, onlyToNew ? "yes" : "no",
                    imbalance(load));
    }

    std::printf("\n3. key ranges vs hashing for keys that grow with time (event ids 0..999999)\n");
    const std::vector<int> lower = {0, 250000, 500000, 750000};   // 4 range shards
    auto rangeShard = [&](int id) {
        const auto it = std::upper_bound(lower.begin(), lower.end(), id);
        return static_cast<int>(it - lower.begin()) - 1;
    };
    std::vector<int> recentRange(4, 0), recentHash(4, 0);
    for (int id = 990000; id < 1000000; ++id) {     // the newest 10,000 events
        ++recentRange[rangeShard(id)];
        ++recentHash[fnv1a("event:" + std::to_string(id)) % 4];
    }
    std::printf("   newest 10000 writes per shard, ranges : %d %d %d %d\n", recentRange[0],
                recentRange[1], recentRange[2], recentRange[3]);
    std::printf("   newest 10000 writes per shard, hashing: %d %d %d %d\n", recentHash[0],
                recentHash[1], recentHash[2], recentHash[3]);
    std::vector<bool> touchedRange(4, false), touchedHash(4, false);
    for (int id = 400000; id < 401000; ++id) {      // a scan of 1,000 consecutive ids
        touchedRange[rangeShard(id)] = true;
        touchedHash[fnv1a("event:" + std::to_string(id)) % 4] = true;
    }
    std::printf("   shards a scan of ids 400000..400999 must ask: ranges %ld, hashing %ld\n",
                std::count(touchedRange.begin(), touchedRange.end(), true),
                std::count(touchedHash.begin(), touchedHash.end(), true));
    return 0;
}
