// A tiny write-back, write-allocate cache WITH DATA in front of a 256-byte memory.
// Two direct-mapped blocks of 16 bytes. The test writes one byte into each of six blocks,
// reads them back through the cache, flushes, and compares memory with what was written.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>

struct Line
{
    bool valid = false;
    bool dirty = false;
    unsigned block = 0;                     // which memory block this line holds
    std::array<std::uint8_t, 16> data{};
};

class WriteBackCache
{
public:
    explicit WriteBackCache(std::array<std::uint8_t, 256>& memory) : mem_(memory) {}

    void write(unsigned address, std::uint8_t value)
    {
        Line& line = lineFor(address);
        line.data[address % 16] = value;
        line.dirty = true;                                      // memory is now out of date
    }

    std::uint8_t read(unsigned address) { return lineFor(address).data[address % 16]; }

    void flush()
    {
        for (Line& line : lines_) {
            if (line.valid && line.dirty) {
                save(line);
            }
        }
    }

private:
    Line& lineFor(unsigned address)
    {
        const unsigned block = address / 16;
        Line& line = lines_[block % 2];
        if (!(line.valid && line.block == block)) {             // miss
            if (line.valid && line.dirty) {                     // write the victim back first
                std::copy_n(line.data.begin(), 16, mem_.begin() + block * 16);
            }
            std::copy_n(mem_.begin() + block * 16, 16, line.data.begin());
            line.valid = true;
            line.dirty = false;
            line.block = block;
        }
        return line;
    }

    void save(Line& line)
    {
        std::copy_n(line.data.begin(), 16, mem_.begin() + line.block * 16);
        line.dirty = false;
    }

    std::array<std::uint8_t, 256>& mem_;
    std::array<Line, 2> lines_{};
};

int main()
{
    std::array<std::uint8_t, 256> memory{};
    WriteBackCache cache(memory);
    for (unsigned i = 0; i < 6; ++i) {
        cache.write(i * 16 + 3, static_cast<std::uint8_t>(10 + i));  // byte 3 of block i
    }
    for (unsigned i = 0; i < 6; ++i) {
        const unsigned value = cache.read(i * 16 + 3);
        std::printf("read  block %u byte 3 through the cache: %u\n", i, value);
    }
    cache.flush();
    int wrong = 0;
    for (unsigned i = 0; i < 6; ++i) {
        const unsigned got = memory[i * 16 + 3];
        const unsigned want = 10 + i;
        std::printf("check memory[0x%02x] = %3u, expected %3u %s\n", i * 16 + 3, got, want,
                    got == want ? "ok" : "WRONG");
        wrong += got == want ? 0 : 1;
    }
    std::printf("%d of 6 checks wrong\n", wrong);
    return 0;
}
