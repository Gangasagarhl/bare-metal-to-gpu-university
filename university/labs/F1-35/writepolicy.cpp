// Count the memory traffic of two write policies on the same trace.
//   write-back + write-allocate     : a write miss first fetches the block; dirty blocks are
//                                     written to memory only when evicted (or flushed at the end)
//   write-through + no-write-allocate: every write goes to memory; a write miss does not fetch
// Cache: direct-mapped, 4 blocks of 16 bytes. Input: lines "R <hex address>" or "W <hex address>".
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Op
{
    char kind;
    std::uint64_t address;
};

struct Traffic
{
    int blockReads = 0;   // 16-byte blocks fetched from memory
    int blockWrites = 0;  // 16-byte blocks written back to memory
    int wordWrites = 0;   // single writes passed straight to memory
};

Traffic simulate(const std::vector<Op>& ops, bool writeBack, bool verbose)
{
    struct Line
    {
        bool valid = false;
        bool dirty = false;
        std::uint64_t tag = 0;
    };
    std::array<Line, 4> lines{};
    Traffic t;
    for (const Op& op : ops) {
        const std::uint64_t block = op.address / 16;
        Line& line = lines[block % 4];
        const bool hit = line.valid && line.tag == block / 4;
        const char* what = hit ? "hit" : "miss";
        if (!hit && (op.kind == 'R' || writeBack)) {          // fetch on read miss, and on write
            if (writeBack && line.valid && line.dirty) {      // miss with write-allocate
                ++t.blockWrites;                              // the victim is dirty: save it first
                what = "miss, dirty victim written back";
            }
            ++t.blockReads;
            line = Line{true, false, block / 4};
        }
        if (op.kind == 'W') {
            if (writeBack) {
                line.dirty = true;                            // memory is now out of date
            } else {
                ++t.wordWrites;                               // write-through: memory updated now
            }
        }
        if (verbose) {
            std::printf("  %c %3llx  %s\n", op.kind, static_cast<unsigned long long>(op.address),
                        what);
        }
    }
    for (const Line& line : lines) {                          // final flush (write-back only)
        if (line.valid && line.dirty) {
            ++t.blockWrites;
        }
    }
    return t;
}

int main()
{
    std::vector<Op> ops;
    char kind = 0;
    std::string word;
    while (std::cin >> kind >> word) {
        ops.push_back(Op{kind, std::stoull(word, nullptr, 16)});
    }
    for (const bool writeBack : {true, false}) {
        std::printf("%s:\n", writeBack ? "write-back + write-allocate"
                                    : "write-through + no-write-allocate");
        const Traffic t = simulate(ops, writeBack, ops.size() <= 16);
        std::printf("  block reads %d, block write-backs %d, single writes %d\n", t.blockReads,
                    t.blockWrites, t.wordWrites);
    }
    return 0;
}
