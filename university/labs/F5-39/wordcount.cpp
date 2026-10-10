// wordcount.cpp - a MapReduce-style word count over file chunks (F5-39, Listing 2).
// A master hands out map tasks (one per input chunk), preferring a worker that stores a replica
// of that chunk; map output is partitioned into R reduce partitions on the worker's local disk;
// reduce tasks fetch their partition from every map worker. One worker dies after its map tasks
// finished: its map output is lost with its disk, so those map tasks run again.
#include <cstdio>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

constexpr int kReduce = 2;   // R: number of reduce partitions

struct Split
{
    std::string text;
    std::set<int> replicas;   // workers whose disk holds this chunk
};

struct Worker
{
    bool alive = true;
    // local disk: map task -> partition -> word -> count
    std::map<int, std::map<int, std::map<std::string, int>>> mapOutput;
};

int partitionOf(const std::string& w)
{
    unsigned h = 0;
    for (char c : w) {
        h = h * 31 + static_cast<unsigned char>(c);
    }
    return static_cast<int>(h % kReduce);
}

// The user's two functions: map emits (word, 1); reduce sums.
void mapFn(const std::string& text, std::map<int, std::map<std::string, int>>& out)
{
    std::istringstream in(text);
    std::string w;
    while (in >> w) {
        out[partitionOf(w)][w] += 1;   // a combiner: sums locally before the shuffle
    }
}

int main()
{
    const std::vector<Split> splits = {
        {"letters go out letters come back", {0, 1}},
        {"friends write letters to friends", {1, 2}},
        {"the post is slow the post is late", {2, 0}},
        {"a letter can be lost", {0, 1}},
        {"friends wait for letters", {1, 2}},
        {"the post comes back", {2, 0}},
    };
    std::vector<Worker> workers(3);
    std::vector<int> doneBy(splits.size(), -1);
    int local = 0;
    int remote = 0;
    int runs = 0;

    auto schedule = [&]() {
        std::vector<int> load(workers.size(), 0);
        for (std::size_t t = 0; t < splits.size(); ++t) {
            if (doneBy[t] >= 0) {
                continue;
            }
            int best = -1;   // prefer an alive worker with a local replica, then the least loaded
            for (int w = 0; w < static_cast<int>(workers.size()); ++w) {
                if (!workers[w].alive) {
                    continue;
                }
                const bool isLocal = splits[t].replicas.count(w) > 0;
                const bool bestLocal = best >= 0 && splits[t].replicas.count(best) > 0;
                if (best < 0 || (isLocal && !bestLocal) || (isLocal == bestLocal && load[w] < load[best])) {
                    best = w;
                }
            }
            const bool isLocal = splits[t].replicas.count(best) > 0;
            (isLocal ? local : remote) += 1;
            ++runs;
            ++load[best];
            mapFn(splits[t].text, workers[best].mapOutput[static_cast<int>(t)]);
            doneBy[t] = best;
            std::printf("map task %zu -> worker %d (%s read)\n", t, best, isLocal ? "local" : "REMOTE");
        }
    };

    std::printf("== map phase (%zu tasks, %d reduce partitions)\n", splits.size(), kReduce);
    schedule();

    std::printf("== worker 2 dies before the reducers fetched its output\n");
    workers[2].alive = false;
    for (std::size_t t = 0; t < splits.size(); ++t) {
        if (doneBy[t] == 2) {
            std::printf("map task %zu was completed on worker 2: its output is lost, run it again\n", t);
            doneBy[t] = -1;
        }
    }
    schedule();

    std::printf("== reduce phase\n");
    for (int r = 0; r < kReduce; ++r) {
        std::map<std::string, int> counts;   // the shuffle: gather partition r from every map task
        for (std::size_t t = 0; t < splits.size(); ++t) {
            for (const auto& [w, n] : workers[doneBy[t]].mapOutput[static_cast<int>(t)][r]) {
                counts[w] += n;
            }
        }
        std::printf("reduce partition %d:", r);
        for (const auto& [w, n] : counts) {
            std::printf(" %s=%d", w.c_str(), n);
        }
        std::printf("\n");
    }
    std::printf("map task runs: %d for %zu tasks; reads local %d, remote %d\n", runs, splits.size(),
                local, remote);
    return 0;
}
