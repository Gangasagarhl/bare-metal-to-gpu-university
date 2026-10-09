// ring.cpp - placing keys on nodes: modulo hashing versus a consistent-hashing ring, with and
// without virtual nodes (F5-40, Listing 1). Counts how evenly 10,000 keys spread over four
// nodes and how many keys move when a fifth node joins. Prints the preference list (the first
// N distinct nodes clockwise) of one key.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>

// 64-bit FNV-1a followed by a final mixing step. Any well-mixed 64-bit hash works here; the
// constants are those of FNV-1a and of the SplitMix64 finaliser (not verified against their
// published definitions in this build; the experiment does not depend on them).
std::uint64_t hash64(const std::string& s)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return h;
}

struct Ring
{
    std::map<std::uint64_t, std::string> tokens;   // position on the ring -> physical node

    void add(const std::string& node, int vnodes)
    {
        for (int v = 0; v < vnodes; ++v) {
            tokens[hash64(node + "#" + std::to_string(v))] = node;
        }
    }
    // the owner of a key: the first token clockwise from the key's position (wrapping around)
    const std::string& owner(const std::string& key) const
    {
        auto it = tokens.lower_bound(hash64(key));
        return (it == tokens.end() ? tokens.begin() : it)->second;
    }
    std::vector<std::string> preferenceList(const std::string& key, std::size_t n) const
    {
        std::vector<std::string> out;
        auto it = tokens.lower_bound(hash64(key));
        for (std::size_t steps = 0; steps < tokens.size() && out.size() < n; ++steps, ++it) {
            if (it == tokens.end()) {
                it = tokens.begin();
            }
            if (std::find(out.begin(), out.end(), it->second) == out.end()) {
                out.push_back(it->second);   // skip further tokens of a node already chosen
            }
        }
        return out;
    }
};

constexpr int kKeys = 10000;

void report(const char* title, const std::vector<std::string>& before, const std::vector<std::string>& after)
{
    std::map<std::string, int> count4;
    std::map<std::string, int> count5;
    int moved = 0;
    for (int k = 0; k < kKeys; ++k) {
        ++count4[before[k]];
        ++count5[after[k]];
        moved += before[k] != after[k];
    }
    auto line = [](const std::map<std::string, int>& c) {
        int mx = 0;
        for (const auto& [n, v] : c) {
            std::printf(" %s=%-5d", n.c_str(), v);
            mx = std::max(mx, v);
        }
        std::printf("  max/mean=%.2f\n", mx / (static_cast<double>(kKeys) / static_cast<double>(c.size())));
    };
    std::printf("%s\n  4 nodes:", title);
    line(count4);
    std::printf("  5 nodes:");
    line(count5);
    std::printf("  keys moved when E joined: %d of %d (%.1f %%); an even share for E would be %d (20.0 %%)\n\n",
                moved, kKeys, 100.0 * moved / kKeys, kKeys / 5);
}

int main()
{
    const std::vector<std::string> four = {"A", "B", "C", "D"};
    const std::vector<std::string> five = {"A", "B", "C", "D", "E"};
    std::vector<std::string> keys;
    for (int k = 0; k < kKeys; ++k) {
        keys.push_back("key-" + std::to_string(k));
    }

    std::vector<std::string> b(kKeys);
    std::vector<std::string> a(kKeys);
    for (int k = 0; k < kKeys; ++k) {
        b[k] = four[hash64(keys[k]) % 4];
        a[k] = five[hash64(keys[k]) % 5];
    }
    report("modulo hashing: node = hash(key) mod number_of_nodes", b, a);

    for (int vn : {1, 64, 256}) {
        Ring r4;
        Ring r5;
        for (const auto& n : four) {
            r4.add(n, vn);
            r5.add(n, vn);
        }
        r5.add("E", vn);
        for (int k = 0; k < kKeys; ++k) {
            b[k] = r4.owner(keys[k]);
            a[k] = r5.owner(keys[k]);
        }
        const std::string title = "consistent hashing, " + std::to_string(vn) + " token(s) per node";
        report(title.c_str(), b, a);
        if (vn == 64) {
            std::printf("preference list of key-42 (N=3), 4 nodes:");
            for (const auto& n : r4.preferenceList("key-42", 3)) {
                std::printf(" %s", n.c_str());
            }
            std::printf("; 5 nodes:");
            for (const auto& n : r5.preferenceList("key-42", 3)) {
                std::printf(" %s", n.c_str());
            }
            std::printf("\n");
        }
    }
    return 0;
}
