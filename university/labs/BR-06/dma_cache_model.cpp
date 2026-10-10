// dma_cache_model.cpp - BR-06: a teaching model of a write-back data cache and a DMA
// device that reads and writes RAM. "Coherent" means the device's accesses are checked
// against the CPU cache (snooped); "non-coherent" means the device sees RAM only.
// Our model, run on the host: it shows what the rules imply, not timing or real hardware.
#include <array>
#include <cstdio>
#include <string>

namespace {

constexpr int kLine = 16;      // bytes per cache line in this model
constexpr int kRam = 64;       // four lines of RAM

struct Line {
    bool valid = false;
    bool dirty = false;
    std::array<char, kLine> data{};
};

struct System {
    std::array<char, kRam> ram{};
    std::array<Line, kRam / kLine> cache{};   // one cache entry per RAM line (model only)
    bool coherent = false;

    // CPU side: every access goes through the cache (write-back, write-allocate).
    Line& fill(int addr)
    {
        Line& l = cache[static_cast<size_t>(addr / kLine)];
        if (!l.valid) {
            for (int i = 0; i < kLine; ++i) {
                l.data[static_cast<size_t>(i)] = ram[static_cast<size_t>(addr / kLine * kLine + i)];
            }
            l.valid = true;
        }
        return l;
    }
    void cpu_write(int addr, const std::string& s)
    {
        for (size_t i = 0; i < s.size(); ++i) {
            const int a = addr + static_cast<int>(i);
            Line& l = fill(a);
            l.data[static_cast<size_t>(a % kLine)] = s[i];
            l.dirty = true;
        }
    }
    std::string cpu_read(int addr, int n)
    {
        std::string s;
        for (int i = 0; i < n; ++i) {
            s += fill(addr + i).data[static_cast<size_t>((addr + i) % kLine)];
        }
        return s;
    }
    // Cache maintenance by address range (what DC CVAC / DC IVAC or cbo.clean / cbo.inval do
    // for one line each, according to their architecture manuals; unverified here).
    void clean(int addr, int n)
    {
        for (int a = addr / kLine * kLine; a < addr + n; a += kLine) {
            Line& l = cache[static_cast<size_t>(a / kLine)];
            if (l.valid && l.dirty) {
                for (int i = 0; i < kLine; ++i) {
                    ram[static_cast<size_t>(a + i)] = l.data[static_cast<size_t>(i)];
                }
                l.dirty = false;
            }
        }
    }
    void invalidate(int addr, int n)
    {
        for (int a = addr / kLine * kLine; a < addr + n; a += kLine) {
            cache[static_cast<size_t>(a / kLine)] = Line{};   // dirty data, if any, is lost
        }
    }
    void evict_all()                                           // the cache decides to write back
    {
        clean(0, kRam);
    }
    // Device side.
    std::string dma_read(int addr, int n)
    {
        if (coherent) {
            clean(addr, n);          // snooping: the device gets the newest data
        }
        return std::string(ram.begin() + addr, ram.begin() + addr + n);
    }
    void dma_write(int addr, const std::string& s)
    {
        if (coherent) {
            invalidate(addr, static_cast<int>(s.size()));
        }
        for (size_t i = 0; i < s.size(); ++i) {
            ram[static_cast<size_t>(addr) + i] = s[i];
        }
    }
};

void tx(bool coherent, bool do_clean)
{
    System m;
    m.coherent = coherent;
    m.ram.fill('.');
    m.cpu_write(0, "PACKET-1");             // driver fills the transmit buffer
    if (do_clean) {
        m.clean(0, 8);
    }
    std::printf("  TX %-13s %-16s device sends \"%s\"\n", coherent ? "coherent" : "non-coherent",
                do_clean ? "clean first" : "no maintenance", m.dma_read(0, 8).c_str());
}

void rx(bool coherent, bool do_inval)
{
    System m;
    m.coherent = coherent;
    m.ram.fill('.');
    (void)m.cpu_read(16, 8);                // the driver looked at the buffer earlier
    m.dma_write(16, "REPLY-42");             // the device receives a packet
    if (do_inval) {
        m.invalidate(16, 8);
    }
    std::printf("  RX %-13s %-16s CPU reads    \"%s\"\n", coherent ? "coherent" : "non-coherent",
                do_inval ? "invalidate after" : "no maintenance", m.cpu_read(16, 8).c_str());
}

void shared_line()
{
    System m;
    m.ram.fill('.');
    m.invalidate(40, 8);                      // driver hands bytes 40..47 to the device, line 32..47 clean
    m.cpu_write(32, "rx:");                   // ... then updates a counter at 32, in the SAME line:
                                              // the line is refilled from RAM and becomes dirty
    m.dma_write(40, "DATA-77!");              // device fills bytes 40..47 in RAM
    m.evict_all();                            // later the cache writes the whole dirty line back
    std::printf("  RX non-coherent, buffer shares a cache line with a counter: RAM bytes 40..47 = \"%s\"\n",
                std::string(m.ram.begin() + 40, m.ram.begin() + 48).c_str());
}

} // namespace

int main()
{
    std::printf("model: %d-byte lines, write-back cache, RAM initially '.'\n", kLine);
    tx(true, false);
    tx(false, false);
    tx(false, true);
    rx(true, false);
    rx(false, false);
    rx(false, true);
    shared_line();
    return 0;
}
