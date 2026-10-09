// counter.cpp - a replicated counter that fails in instructive ways (DS301 course lab, F5-12).
// Six designs of "three copies of one counter", each run in a small deterministic simulation
// with an unreliable network (messages lost or duplicated) or a crash. The program prints the
// intended value and what every replica ends up with.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <set>
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
    bool chance(double p) { return uniform() < p; }
    int below(int n) { return static_cast<int>(uniform() * n); }
};

constexpr double kLoss = 0.10;   // probability that one message is lost
constexpr double kDup = 0.05;    // probability that one delivered message arrives twice

void report(const char* name, int intended, const std::vector<long>& values, const char* note)
{
    std::printf("%-40s intended %3d  replicas:", name, intended);
    for (long v : values) {
        std::printf(" %4ld", v);
    }
    const bool same = std::all_of(values.begin(), values.end(),
                                  [&](long v) { return v == values[0]; });
    const bool right = same && values[0] == intended;
    const char* verdict = right ? "CORRECT"
                          : values.size() == 1 ? "WRONG"
                          : same ? "WRONG (but replicas agree)" : "WRONG and replicas DIVERGE";
    std::printf("   %s\n    %s\n", verdict, note);
}

// 1 and 2: the client sends "increment" to each replica and resends until it sees an ack.
void incrementMessages(bool dedup, Rng& rng)
{
    std::array<long, 3> value{};
    std::array<std::set<int>, 3> seen;
    for (int op = 1; op <= 100; ++op) {
        for (int r = 0; r < 3; ++r) {
            for (bool acked = false; !acked;) {
                if (rng.chance(kLoss)) {
                    continue;                                   // request lost: resend
                }
                const int copies = rng.chance(kDup) ? 2 : 1;    // the network may duplicate
                for (int c = 0; c < copies; ++c) {
                    if (!dedup || seen[r].insert(op).second) {
                        ++value[r];
                    }
                }
                acked = !rng.chance(kLoss);                     // the ack may be lost too
            }
        }
    }
    report(dedup ? "2 increments + retry + op-id dedup" : "1 increments + retry (at-least-once)",
           100, {value[0], value[1], value[2]},
           dedup ? "each replica remembers op ids it applied: a resend changes nothing"
                 : "a lost ack causes a resend of an increment that was already applied");
}

// 3: two clients each do 100 times: read the value from a replica, add 1, write the value back.
void readModifyWrite(Rng& rng)
{
    long value = 0;           // all replicas get every write here: only the race matters
    int lost = 0;
    for (int i = 0; i < 100; ++i) {
        if (rng.chance(0.3)) {   // both clients read before either writes
            const long a = value;
            const long b = value;
            value = a + 1;
            value = b + 1;
            ++lost;
        } else {
            value = value + 1;
            value = value + 1;
        }
    }
    report("3 read, add 1, write the value back", 200, {value, value, value},
           ("two clients raced " + std::to_string(lost) + " times; each race lost one update")
               .c_str());
}

// 4: G-counter: each replica counts its own increments; replicas exchange their whole vector
// over the lossy network and merge with max. Merge ignores duplicates and old messages.
void gCounter(Rng& rng)
{
    using Vec = std::array<long, 3>;
    std::array<Vec, 3> rep{};
    auto merge = [](Vec& into, const Vec& from) {
        for (int i = 0; i < 3; ++i) {
            into[i] = std::max(into[i], from[i]);
        }
    };
    auto total = [](const Vec& v) { return v[0] + v[1] + v[2]; };
    for (int op = 0; op < 200; ++op) {
        const int r = rng.below(3);
        ++rep[r][r];                                  // an increment at any replica
        const int from = rng.below(3);
        const int to = (from + 1 + rng.below(2)) % 3;
        if (!rng.chance(kLoss)) {
            const int copies = rng.chance(kDup) ? 2 : 1;
            for (int c = 0; c < copies; ++c) {
                merge(rep[to], rep[from]);
            }
        }
    }
    report("4 G-counter, before the final exchange", 200,
           {total(rep[0]), total(rep[1]), total(rep[2])},
           "replicas lag behind each other while messages are lost: not yet converged");
    int rounds = 0;
    while (!(rep[0] == rep[1] && rep[1] == rep[2])) {
        ++rounds;
        const int from = rng.below(3);
        const int to = (from + 1 + rng.below(2)) % 3;
        if (!rng.chance(kLoss)) {
            merge(rep[to], rep[from]);
        }
    }
    report("4 G-counter, after anti-entropy", 200, {total(rep[0]), total(rep[1]), total(rep[2])},
           ("converged after " + std::to_string(rounds) + " more exchanges").c_str());
}

// 5, 6 and 7: primary-backup. The primary crashes after acknowledging op 70; the backup
// takes over and the client sends ops 71..100 to it.
void primaryBackup(bool sync, bool dedup)
{
    long backup = 0;
    std::set<int> backupSeen;
    std::vector<int> inFlight;             // async: forwarded but not yet applied by the backup
    auto applyAtBackup = [&](int op) {
        if (!dedup || backupSeen.insert(op).second) {
            ++backup;
        }
    };
    for (int op = 1; op <= 70; ++op) {
        // (the primary applies op locally first; only the backup's copy matters after the crash)
        if (sync) {
            applyAtBackup(op);             // forward, wait for the backup's ack, then ack client
        } else {
            inFlight.push_back(op);        // ack the client first, forward later
            if (op % 8 == 0) {             // the primary ships its changes in batches of 8
                for (int f : inFlight) {
                    applyAtBackup(f);
                }
                inFlight.clear();
            }
        }
    }
    int lostAcked = static_cast<int>(inFlight.size());   // async: died with the primary
    if (sync) {
        // op 71 reached the backup, then the primary crashed before acking the client;
        // the client times out and resends op 71 to the new primary
        applyAtBackup(71);
    }
    for (int op = 71; op <= 100; ++op) {
        applyAtBackup(op);                 // the backup is now the primary
    }
    std::string note;
    if (!sync) {
        note = std::to_string(lostAcked) + " increments were acknowledged to the client but "
               "had not reached the backup when the primary crashed";
    } else if (!dedup) {
        note = "op 71 was applied at the backup, its ack was lost in the crash, and the "
               "client's resend applied it again";
    } else {
        note = "the resend of op 71 is recognised by its op id: every acknowledged increment "
               "survives, none counts twice";
    }
    const char* name = !sync ? "5 primary-backup, asynchronous"
                       : !dedup ? "6 primary-backup, synchronous"
                                : "7 primary-backup, synchronous + dedup";
    report(name, 100, {backup}, note.c_str());
}

int main()
{
    Rng rng{2026};
    std::printf("network: %.0f %% of messages lost, %.0f %% of delivered messages duplicated\n\n",
                kLoss * 100, kDup * 100);
    incrementMessages(false, rng);
    incrementMessages(true, rng);
    readModifyWrite(rng);
    gCounter(rng);
    std::printf("\n(primary-backup rows show the one surviving copy, the promoted backup)\n");
    primaryBackup(false, false);
    primaryBackup(true, false);
    primaryBackup(true, true);
    return 0;
}
