// gfs.cpp - a GFS-style distributed file system squeezed into one process (F5-39, Listing 1).
// One master keeps the metadata (files -> chunks -> replica locations and versions); five
// chunk servers keep the bytes. The run shows: chunk placement, leases and version numbers,
// record append with a failed secondary and a client retry (a duplicate record), a reader that
// skips padding and duplicates, a chunk server that dies and is re-replicated, and a stale
// replica rejected by its version number. Sizes and times are exercise values, not GFS's.
#include <algorithm>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

constexpr int kChunkSize = 36;        // bytes per chunk in this toy
constexpr int kReplicas = 3;          // replication target
constexpr int kServers = 5;
constexpr int kLeaseMs = 10000;       // lease length
constexpr int kDeadAfterMs = 3000;    // missed-heartbeat limit

int now = 0;                          // simulated clock in ms

void log(const char* who, const std::string& msg)
{
    std::printf("[t=%6d ms] %-8s %s\n", now, who, msg.c_str());
}

struct Replica
{
    int version = 0;
    std::string bytes;
};

struct ChunkServer
{
    bool up = true;
    int lastHeartbeat = 0;
    std::map<int, Replica> chunks;
};

struct ChunkInfo
{
    int version = 0;
    std::vector<int> locations;
    int primary = -1;
    int leaseUntil = -1;
};

std::vector<ChunkServer> cs(kServers);
std::map<std::string, std::vector<int>> files;   // master: namespace
std::map<int, ChunkInfo> chunks;                  // master: chunk table
int nextHandle = 100;
int failNextForwardTo = -1;                       // fault injection: drop one forward

std::string name(int s) { return "cs" + std::to_string(s); }

std::string list(const std::vector<int>& v)
{
    std::string out;
    for (int s : v) {
        out += (out.empty() ? "" : ",") + name(s);
    }
    return out;
}

int newChunk(const std::string& path)
{
    std::vector<int> cand;
    for (int s = 0; s < kServers; ++s) {
        if (cs[s].up) {
            cand.push_back(s);
        }
    }
    // least-loaded servers first, ties by number: spreads chunks deterministically
    std::stable_sort(cand.begin(), cand.end(), [](int a, int b) {
        return cs[a].chunks.size() < cs[b].chunks.size();
    });
    cand.resize(kReplicas);
    const int h = nextHandle++;
    chunks[h].locations = cand;
    for (int s : cand) {
        cs[s].chunks[h] = Replica{};
    }
    files[path].push_back(h);
    log("master", "new chunk " + std::to_string(h) + " of " + path + " on " + list(cand));
    return h;
}

int grantLease(int h)
{
    ChunkInfo& c = chunks[h];
    if (c.primary >= 0 && now < c.leaseUntil) {
        return c.primary;
    }
    c.version += 1;   // a new lease means a new version: replicas that miss it become stale
    c.primary = c.locations.front();
    c.leaseUntil = now + kLeaseMs;
    for (int s : c.locations) {
        cs[s].chunks[h].version = c.version;
    }
    log("master", "lease on chunk " + std::to_string(h) + " to " + name(c.primary) + ", version "
                  + std::to_string(c.version) + ", replicas " + list(c.locations));
    return c.primary;
}

// Record append: the primary picks the offset, every replica writes at that same offset.
// Returns false if any replica failed; the client must then retry the whole append.
bool appendOnce(const std::string& path, const std::string& rec)
{
    int h = files[path].back();
    int p = grantLease(h);
    Replica& pr = cs[p].chunks[h];
    if (static_cast<int>(pr.bytes.size() + rec.size()) > kChunkSize) {
        // does not fit: pad every replica to the chunk end, then use a new chunk
        for (int s : chunks[h].locations) {
            cs[s].chunks[h].bytes.resize(kChunkSize, '.');
        }
        log(name(p).c_str(), "chunk " + std::to_string(h) + " has no room: padded to "
                             + std::to_string(kChunkSize) + " bytes, append moves to a new chunk");
        h = newChunk(path);
        p = grantLease(h);
    }
    const std::size_t off = cs[p].chunks[h].bytes.size();
    bool ok = true;
    for (int s : chunks[h].locations) {
        if (s == failNextForwardTo) {
            failNextForwardTo = -1;
            ok = false;
            log(name(p).c_str(), "forward of '" + rec.substr(0, rec.size() - 1) + "' to " + name(s)
                                 + " failed");
            continue;
        }
        std::string& b = cs[s].chunks[h].bytes;
        if (b.size() < off) {
            b.resize(off, '.');   // a replica that missed an earlier write pads the gap
        }
        b.replace(off, rec.size(), rec);
    }
    log(name(p).c_str(), "append '" + rec.substr(0, rec.size() - 1) + "' at chunk "
                         + std::to_string(h) + " offset " + std::to_string(off)
                         + (ok ? ": ok" : ": ERROR to client"));
    return ok;
}

void recordAppend(const std::string& path, const std::string& rec)
{
    for (int attempt = 1; !appendOnce(path, rec); ++attempt) {
        log("client", "retry " + std::to_string(attempt) + " of append");
    }
}

void showReplicas(const std::string& path)
{
    for (int h : files[path]) {
        std::printf("    chunk %d (master version %d):\n", h, chunks[h].version);
        for (int s = 0; s < kServers; ++s) {
            auto it = cs[s].chunks.find(h);
            if (it != cs[s].chunks.end()) {
                std::printf("      %s v%d%s |%s|\n", name(s).c_str(), it->second.version,
                            cs[s].up ? "     " : " DOWN", it->second.bytes.c_str());
            }
        }
    }
}

// A reader: fetch each chunk from the first listed replica, split into records,
// skip padding and records whose id it has already seen.
void readFile(const std::string& path)
{
    std::set<std::string> seen;
    std::string out;
    int dups = 0;
    for (int h : files[path]) {
        const int s = chunks[h].locations.front();
        const std::string& b = cs[s].chunks[h].bytes;
        std::size_t i = 0;
        while (i < b.size()) {
            if (b[i] == '.') {
                ++i;
                continue;
            }
            const std::size_t end = b.find(';', i);
            const std::string rec = b.substr(i, end - i);
            const std::string id = rec.substr(0, rec.find(':'));
            if (seen.insert(id).second) {
                out += rec + " ";
            } else {
                ++dups;
            }
            i = end + 1;
        }
    }
    log("reader", "records: " + out + "(duplicates skipped: " + std::to_string(dups) + ")");
}

void heartbeats()
{
    for (int s = 0; s < kServers; ++s) {
        if (cs[s].up) {
            cs[s].lastHeartbeat = now;
        }
    }
}

void masterScan()
{
    for (int s = 0; s < kServers; ++s) {
        bool listed = false;
        for (auto& [h, c] : chunks) {
            listed = listed || std::count(c.locations.begin(), c.locations.end(), s) > 0;
        }
        if (listed && now - cs[s].lastHeartbeat > kDeadAfterMs) {
            log("master", name(s) + " missed heartbeats for " + std::to_string(now - cs[s].lastHeartbeat)
                          + " ms: removed from all chunk locations");
            for (auto& [h, c] : chunks) {
                std::erase(c.locations, s);
                if (c.primary == s) {
                    c.primary = -1;
                }
            }
        }
    }
    for (auto& [h, c] : chunks) {
        while (static_cast<int>(c.locations.size()) < kReplicas) {
            int dst = -1;
            for (int s = 0; s < kServers; ++s) {
                const bool has = std::count(c.locations.begin(), c.locations.end(), s) > 0;
                if (cs[s].up && !has && (dst < 0 || cs[s].chunks.size() < cs[dst].chunks.size())) {
                    dst = s;
                }
            }
            const int src = c.locations.front();
            cs[dst].chunks[h] = cs[src].chunks[h];
            c.locations.push_back(dst);
            log("master", "re-replicate chunk " + std::to_string(h) + ": " + name(src) + " -> "
                          + name(dst) + " (now " + list(c.locations) + ")");
        }
    }
}

// A server that comes back reports what it holds; the master compares versions.
void rejoin(int s)
{
    cs[s].up = true;
    cs[s].lastHeartbeat = now;
    for (auto it = cs[s].chunks.begin(); it != cs[s].chunks.end();) {
        const int h = it->first;
        const int v = it->second.version;
        ChunkInfo& c = chunks[h];
        const bool listed = std::count(c.locations.begin(), c.locations.end(), s) > 0;
        if (v < c.version) {
            log("master", name(s) + " reports chunk " + std::to_string(h) + " v" + std::to_string(v)
                          + " < current v" + std::to_string(c.version) + ": STALE, deleted");
            it = cs[s].chunks.erase(it);
            continue;
        }
        if (!listed) {
            log("master", name(s) + " reports chunk " + std::to_string(h) + " v" + std::to_string(v)
                          + ": current, but already " + std::to_string(c.locations.size())
                          + " replicas: extra copy deleted");
            it = cs[s].chunks.erase(it);
            continue;
        }
        ++it;
    }
}

int main()
{
    const std::string f = "/logs/sensors";
    std::printf("== 1. create a file and append records (chunk size %d bytes, %d replicas)\n",
                kChunkSize, kReplicas);
    newChunk(f);
    recordAppend(f, "r1:t=21;");
    now += 5;
    recordAppend(f, "r2:t=22;");
    std::printf("== 2. a forward to one secondary fails; the client retries the append\n");
    now += 5;
    failNextForwardTo = chunks[files[f].back()].locations.back();
    recordAppend(f, "r3:t=23;");
    now += 5;
    recordAppend(f, "r4:t=25;");
    now += 5;
    recordAppend(f, "r5:t=24;");
    showReplicas(f);
    readFile(f);

    std::printf("== 3. cs0 stops sending heartbeats; writes go on without it\n");
    cs[0].up = false;
    const int deadServer = 0;
    for (int step = 0; step < 4; ++step) {
        now += 1000;
        heartbeats();
        masterScan();
    }
    now += 20000;   // the old lease has expired; the next write needs a new lease
    heartbeats();
    recordAppend(f, "r6:t=26;");
    showReplicas(f);

    std::printf("== 4. cs0 comes back with the chunks it had when it died\n");
    now += 1000;
    rejoin(deadServer);
    showReplicas(f);
    readFile(f);
    return 0;
}
