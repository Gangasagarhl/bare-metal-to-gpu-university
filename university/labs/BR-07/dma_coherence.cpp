// dma_coherence.cpp - BR-07: why non-coherent DMA needs cache maintenance.
//
// This course's own model (not a real cache): memory of 64 bytes, a write-back CPU cache with
// 16-byte lines, and a DMA device that reads and writes memory. A non-coherent device sees
// only memory; a coherent one also sees (snoops) the CPU cache. Real caches have more levels,
// other line sizes and their own rules; the order of the operations is the lesson.
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>

namespace {

constexpr int kLine = 16;
constexpr int kLines = 4;

struct System {
    std::array<char, kLine * kLines> memory{};
    std::array<std::array<char, kLine>, kLines> cache{};
    std::array<bool, kLines> valid{};
    std::array<bool, kLines> dirty{};
    bool coherent = false;               // true: the devicetree node says dma-coherent

    void fill(int l)
    {
        for (int i = 0; i < kLine; ++i) {
            cache[l][i] = memory[l * kLine + i];
        }
        valid[l] = true;
        dirty[l] = false;
    }
    void cpu_write(int a, char v)
    {
        int l = a / kLine;
        if (!valid[l]) {
            fill(l);
        }
        cache[l][a % kLine] = v;         // write-back: memory is NOT updated now
        dirty[l] = true;
    }
    char cpu_read(int a)
    {
        int l = a / kLine;
        if (!valid[l]) {
            fill(l);
        }
        return cache[l][a % kLine];
    }
    void clean(int l)                    // write a dirty line back to memory
    {
        if (valid[l] && dirty[l]) {
            for (int i = 0; i < kLine; ++i) {
                memory[l * kLine + i] = cache[l][i];
            }
            dirty[l] = false;
        }
    }
    void invalidate(int l) { valid[l] = false; }   // forget the line (clean first if needed)
    char dev_read(int a)
    {
        int l = a / kLine;
        if (coherent && valid[l]) {
            return cache[l][a % kLine];  // the interconnect finds the newest copy
        }
        return memory[a];
    }
    void dev_write(int a, char v)
    {
        int l = a / kLine;
        memory[a] = v;
        if (coherent && valid[l]) {
            cache[l][a % kLine] = v;     // a coherent write also updates the cached copy
        }
    }
};

std::string printable(const std::string& s)
{
    std::string out;
    for (char c : s) {
        out += c == '\0' ? '.' : c;
    }
    return out;
}

// CPU writes "HELLO" into line 0, the device sends it (TX). Then the device receives "WORLD"
// into line 1 that the CPU had read before (RX).
bool scenario(const char* title, bool coherent, bool maintain)
{
    System s;
    s.coherent = coherent;
    const std::string tx = "HELLO";
    for (int i = 0; i < 5; ++i) {
        s.cpu_write(i, tx[static_cast<size_t>(i)]);
    }
    if (maintain) {
        s.clean(0);                      // before the device reads: clean
    }
    std::string sent;
    for (int i = 0; i < 5; ++i) {
        sent += s.dev_read(i);
    }
    (void)s.cpu_read(kLine);             // the CPU looked at the RX buffer earlier
    const std::string rx = "WORLD";
    if (maintain) {
        s.invalidate(1);                 // before (or after) the device writes: invalidate
    }
    for (int i = 0; i < 5; ++i) {
        s.dev_write(kLine + i, rx[static_cast<size_t>(i)]);
    }
    std::string got;
    for (int i = 0; i < 5; ++i) {
        got += s.cpu_read(kLine + i);
    }
    bool ok = sent == tx && got == rx;
    std::printf("%-40s device sent \"%s\", CPU received \"%s\"  %s\n", title,
                printable(sent).c_str(), printable(got).c_str(), ok ? "ok" : "WRONG");
    return ok;
}

}  // namespace

int main()
{
    bool a = scenario("PC habit, non-coherent, no maintenance:", false, false);
    bool b = scenario("non-coherent, clean + invalidate:", false, true);
    bool c = scenario("coherent (dma-coherent), no maintenance:", true, false);
    std::printf("expected: WRONG, ok, ok -> %s\n", (!a && b && c) ? "as expected" : "UNEXPECTED");
    return (!a && b && c) ? 0 : 1;
}
