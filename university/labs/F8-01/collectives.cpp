// F8-01 Listing 1: the six collectives, written as plain C++ on simulated ranks.
// Each "rank" is one std::vector<int> (its buffer). Every function returns the buffers
// as they are after the collective, so you can compare before and after.
#include <cstdio>
#include <string>
#include <vector>

using Buffers = std::vector<std::vector<int>>;   // buffers[rank][element]

void show(const char* title, const Buffers& b)
{
    std::printf("%s\n", title);
    for (std::size_t r = 0; r < b.size(); ++r) {
        std::string line = "  rank " + std::to_string(r) + ":";
        for (int v : b[r]) {
            line += " " + std::to_string(v);
        }
        std::printf("%s\n", line.c_str());
    }
}

// broadcast: every rank ends with the root's buffer
Buffers broadcast(const Buffers& in, std::size_t root)
{
    return Buffers(in.size(), in[root]);
}

// reduce: the root ends with the element-wise sum; other buffers are unchanged
Buffers reduce(const Buffers& in, std::size_t root)
{
    Buffers out = in;
    std::vector<int> sum(in[0].size(), 0);
    for (const auto& b : in) {
        for (std::size_t i = 0; i < b.size(); ++i) {
            sum[i] += b[i];
        }
    }
    out[root] = sum;
    return out;
}

// all-reduce: every rank ends with the element-wise sum
Buffers allReduce(const Buffers& in)
{
    return broadcast(reduce(in, 0), 0);
}

// reduce-scatter: rank r ends with block r of the sum (block = count / ranks elements)
Buffers reduceScatter(const Buffers& in)
{
    const std::size_t n = in.size();
    const std::size_t block = in[0].size() / n;
    const std::vector<int> sum = reduce(in, 0)[0];
    Buffers out(n);
    for (std::size_t r = 0; r < n; ++r) {
        out[r].assign(sum.begin() + static_cast<long>(r * block),
                      sum.begin() + static_cast<long>((r + 1) * block));
    }
    return out;
}

// all-gather: every rank ends with all ranks' buffers, concatenated in rank order
Buffers allGather(const Buffers& in)
{
    std::vector<int> all;
    for (const auto& b : in) {
        all.insert(all.end(), b.begin(), b.end());
    }
    return Buffers(in.size(), all);
}

// all-to-all: block j of rank i's buffer goes to block i of rank j's buffer
Buffers allToAll(const Buffers& in)
{
    const std::size_t n = in.size();
    const std::size_t block = in[0].size() / n;
    Buffers out(n, std::vector<int>(in[0].size(), 0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t k = 0; k < block; ++k) {
                out[j][i * block + k] = in[i][j * block + k];
            }
        }
    }
    return out;
}

int main()
{
    const std::size_t ranks = 4;
    const std::size_t count = 4;                 // elements per rank
    Buffers start(ranks, std::vector<int>(count));
    for (std::size_t r = 0; r < ranks; ++r) {
        for (std::size_t i = 0; i < count; ++i) {
            start[r][i] = static_cast<int>(10 * r + i);  // rank 2 holds 20 21 22 23
        }
    }
    show("before (every collective starts from these buffers):", start);
    show("broadcast from root 0:", broadcast(start, 0));
    show("reduce (sum) to root 0:", reduce(start, 0));
    show("all-reduce (sum):", allReduce(start));
    show("reduce-scatter (sum), one block of 1 element per rank:", reduceScatter(start));
    show("all-gather (every rank contributes its 4 elements):", allGather(start));
    show("all-to-all (block j of rank i goes to rank j):", allToAll(start));

    // the identity used by the ring algorithm (F8-02): all-reduce = reduce-scatter + all-gather
    const Buffers composed = allGather(reduceScatter(start));
    std::printf("all-gather(reduce-scatter(x)) == all-reduce(x): %s\n",
                composed == allReduce(start) ? "yes" : "NO");
    return composed == allReduce(start) ? 0 : 1;
}
