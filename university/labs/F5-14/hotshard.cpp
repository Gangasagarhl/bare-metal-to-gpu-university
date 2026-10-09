// hotshard.cpp - generates the evidence pack of the F5-14 forensic lab "more shards, same fire".
// A "likes" service keeps one counter per post, hash-partitioned over shards. Operators saw
// one shard overloaded and doubled the number of shards at minute 4. The cause is in the key.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

struct Rng
{
    std::uint64_t state;
    std::uint64_t next()
    {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
};

std::uint64_t hashKey(const std::string& s)   // FNV-1a 64 with a final mix (as in sharding.cpp)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    return h;
}

int main()
{
    Rng rng{99};
    const int perMinute = 60000;
    std::printf("=== shard_load.log: requests per minute per shard ===\n");
    std::map<std::string, int> topKeys;     // sampled on the busiest shard in the last minute
    for (int minute = 1; minute <= 6; ++minute) {
        const int shards = minute < 4 ? 8 : 16;
        std::vector<int> load(shards, 0);
        std::vector<std::string> keys(perMinute);
        for (int i = 0; i < perMinute; ++i) {
            std::string key;
            if (rng.uniform() < 0.35) {
                key = "post:7781203";                                  // the planted hot key
            } else {
                key = "post:" + std::to_string(static_cast<int>(rng.uniform() * 9000000));
            }
            ++load[hashKey(key) % shards];
            keys[i] = key;
        }
        const auto top = std::max_element(load.begin(), load.end());
        const int busiest = static_cast<int>(top - load.begin());
        std::printf("12:%02d  %2d shards |", minute, shards);
        for (int s = 0; s < shards; ++s) {
            std::printf(" %5d", load[s]);
        }
        std::printf("\n             busiest: shard %d with %d (%.0f %% of all requests)\n", busiest,
                    load[busiest], 100.0 * load[busiest] / perMinute);
        if (minute == 6) {
            for (const std::string& k : keys) {
                if (static_cast<int>(hashKey(k) % shards) == busiest) {
                    ++topKeys[k];
                }
            }
        }
    }
    std::vector<std::pair<int, std::string>> top;
    for (const auto& [k, c] : topKeys) {
        top.push_back({c, k});
    }
    std::sort(top.rbegin(), top.rend());
    std::printf("\n=== key sample: busiest shard, 12:06, top 5 keys by requests ===\n");
    for (int i = 0; i < 5 && i < static_cast<int>(top.size()); ++i) {
        std::printf("%-14s %6d\n", top[i].second.c_str(), top[i].first);
    }
    std::printf("\n=== ops note ===\n12:03:40 shard count doubled 8 -> 16 (keys re-hashed, "
                "data moved); alert still firing at 12:06\n");
    return 0;
}
