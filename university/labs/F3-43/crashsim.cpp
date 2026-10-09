// crashsim.cpp - F3-43: every crash state of "append one block to a file", for five
// write orderings. The disk has a volatile write cache: a write is only durable after a
// later FLUSH; writes issued since the last FLUSH may reach the medium in any subset.
#include <cstdio>
#include <map>
#include <string>
#include <vector>

enum class V { Old, New, Stale };             // what a block on the medium contains

struct Op
{
    std::string block;                        // "D", "I", "B", "J1", "J2", "JC" or "" = FLUSH
};

struct Strategy
{
    std::string name;
    std::vector<Op> ops;
    bool journal;                             // recovery replays J1/J2 when JC is present
    bool commitChecksum;                      // replay only if J1 and J2 are both intact
};

using Disk = std::map<std::string, V>;

Disk initialDisk()
{
    // D holds stale bytes of a deleted file; the journal area holds an old transaction.
    return { {"D", V::Old}, {"I", V::Old}, {"B", V::Old},
             {"J1", V::Stale}, {"J2", V::Stale}, {"JC", V::Old} };
}

void recover(Disk& d, const Strategy& s)
{
    if (!s.journal || d["JC"] != V::New) {
        return;                               // no committed transaction: nothing to replay
    }
    if (s.commitChecksum && (d["J1"] != V::New || d["J2"] != V::New)) {
        return;                               // checksum mismatch: transaction ignored
    }
    d["I"] = d["J1"] == V::New ? V::New : V::Stale;   // replay copies whatever is logged
    d["B"] = d["J2"] == V::New ? V::New : V::Stale;
}

std::string classify(Disk& d)
{
    if (d["I"] == V::Stale || d["B"] == V::Stale) {
        return "CORRUPT: replay wrote stale journal blocks over metadata";
    }
    std::string bad;
    if (d["I"] == V::New && d["B"] == V::Old) {
        bad += "BAD: block used by the file but free in the bitmap; ";
    }
    if (d["I"] == V::New && d["D"] == V::Old) {
        bad += "BAD: file shows stale data of a deleted file; ";
    }
    if (!bad.empty()) {
        return bad.substr(0, bad.size() - 2);
    }
    if (d["I"] == V::Old && d["B"] == V::New) {
        return "ok-leak: block marked used but owned by nobody (fsck reclaims it)";
    }
    return d["I"] == V::New ? "ok: new file contents" : "ok: old file contents";
}

void explore(const Strategy& s)
{
    std::map<std::string, int> outcomes;
    int states = 0;
    for (std::size_t k = 0; k <= s.ops.size(); ++k) {          // crash after k operations
        std::vector<std::string> durable;
        std::vector<std::string> open;
        for (std::size_t i = 0; i < k; ++i) {
            if (s.ops[i].block.empty()) {                       // FLUSH: open writes durable
                durable.insert(durable.end(), open.begin(), open.end());
                open.clear();
            } else {
                open.push_back(s.ops[i].block);
            }
        }
        for (unsigned mask = 0; mask < (1u << open.size()); ++mask) {
            Disk d = initialDisk();
            for (const auto& b : durable) {
                d[b] = V::New;
            }
            for (std::size_t j = 0; j < open.size(); ++j) {
                if (mask & (1u << j)) {
                    d[open[j]] = V::New;                        // this cached write landed
                }
            }
            recover(d, s);
            ++outcomes[classify(d)];
            ++states;
        }
    }
    std::printf("== %s ==\n   order:", s.name.c_str());
    for (const auto& op : s.ops) {
        std::printf(" %s", op.block.empty() ? "FLUSH" : ("W(" + op.block + ")").c_str());
    }
    std::printf("\n   crash states examined: %d\n", states);
    for (const auto& [what, n] : outcomes) {
        std::printf("   %4d  %s\n", n, what.c_str());
    }
}

int main()
{
    const Op F{""};
    const std::vector<Strategy> all = {
        {"1 no ordering", {{"I"}, {"B"}, {"D"}, F}, false, false},
        {"2 careful order: data and bitmap, flush, then inode", {{"D"}, {"B"}, F, {"I"}, F},
         false, false},
        {"3 journal (ordered mode): data, log, flush, commit, flush, checkpoint",
         {{"D"}, {"J1"}, {"J2"}, F, {"JC"}, F, {"I"}, {"B"}, F}, true, false},
        {"4 journal with the flush before the commit block missing",
         {{"D"}, {"J1"}, {"J2"}, {"JC"}, F, {"I"}, {"B"}, F}, true, false},
        {"5 as 4, but the commit block carries a checksum of the log blocks",
         {{"D"}, {"J1"}, {"J2"}, {"JC"}, F, {"I"}, {"B"}, F}, true, true},
    };
    for (const auto& s : all) {
        explore(s);
    }
    return 0;
}
