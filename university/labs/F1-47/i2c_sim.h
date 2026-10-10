// F1-47: a sample-by-sample model of an I2C bus with one controller and one
// target, plus a decoder that reads the recorded samples back, the way a logic
// analyser's protocol decoder does. Both lines are open-drain: a line is low if
// ANY side pulls it low, high (through the pull-up resistor) otherwise.
// The target is a made-up sensor at 7-bit address 0x48 whose register 0x00
// holds the two bytes 0x19 0x40 (exercise values, not a real part).
#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Sample
{
    int scl;
    int sda;
};

class Bus
{
public:
    std::vector<Sample> trace;
    int scl = 1;               // controller's SCL output (1 = released)
    int sda_ctrl = 1;          // controller's SDA output
    int sda_target = 1;        // target's SDA output
    void tick() { trace.push_back({scl, sda_ctrl & sda_target}); }   // wired-AND
    int sda() const { return sda_ctrl & sda_target; }
};

class Target
{
public:
    std::uint8_t address = 0x48;
    std::uint8_t reg = 0;
    std::uint8_t regs[2] = {0x19, 0x40};
    bool selected = false;
};

// The controller side: each bit is four samples (SCL low, low, high, high).
class Controller
{
public:
    Bus& bus;
    Target& t;
    explicit Controller(Bus& b, Target& tg) : bus(b), t(tg) {}

    void start()
    {
        if (bus.scl == 0) {                    // repeated START: release SDA while SCL
            bus.sda_ctrl = 1; bus.tick();      // is low, then raise SCL
            bus.scl = 1; bus.tick();
        } else {
            bus.sda_ctrl = 1; bus.tick();      // bus idle: both lines high
        }
        bus.sda_ctrl = 0; bus.tick();          // SDA falls while SCL is high
        bus.scl = 0; bus.tick();
    }
    void stop()
    {
        bus.sda_ctrl = 0; bus.scl = 0; bus.tick();
        bus.scl = 1; bus.tick();
        bus.sda_ctrl = 1; bus.tick();          // SDA rises while SCL is high
        bus.tick();
    }
    void clock_bit()
    {
        bus.scl = 0; bus.tick(); bus.tick();
        bus.scl = 1; bus.tick(); bus.tick();
        bus.scl = 0;
    }
    // Send one byte, MSB first; return true if the 9th bit was ACK (low).
    bool write_byte(std::uint8_t b, bool is_address)
    {
        for (int i = 7; i >= 0; --i) {
            bus.sda_ctrl = (b >> i) & 1;
            clock_bit();
        }
        bus.sda_ctrl = 1;                      // release SDA for the ACK bit
        bool ack = false;
        if (is_address) {
            t.selected = (b >> 1) == t.address;
            ack = t.selected;
        } else if (t.selected) {
            t.reg = b;                         // a write after the address sets the pointer
            ack = true;
        }
        bus.sda_target = ack ? 0 : 1;
        clock_bit();
        bus.sda_target = 1;
        return ack;
    }
    std::uint8_t read_byte(bool ack)
    {
        std::uint8_t v = 0;
        const std::uint8_t out = t.regs[t.reg & 1];
        bus.sda_ctrl = 1;
        for (int i = 7; i >= 0; --i) {
            bus.sda_target = (out >> i) & 1;   // the target drives SDA
            clock_bit();
            v = static_cast<std::uint8_t>((v << 1) | bus.trace.back().sda);
        }
        bus.sda_target = 1;
        ++t.reg;
        bus.sda_ctrl = ack ? 0 : 1;            // controller ACKs all but the last byte
        clock_bit();
        bus.sda_ctrl = 1;
        return v;
    }
};

// Read `n` bytes from register `reg` of 7-bit address `addr7`: write the
// register pointer, repeated START, then read. Returns false on a NACK.
inline bool read_reg(Controller& c, unsigned addr7, std::uint8_t reg, int n,
                     std::vector<std::uint8_t>& out)
{
    c.start();
    const auto wr = static_cast<std::uint8_t>(addr7 << 1);          // R/W bit 0 = write
    if (!c.write_byte(wr, true)) {
        c.stop();
        return false;
    }
    c.write_byte(reg, false);
    c.start();                                                      // repeated START
    c.write_byte(static_cast<std::uint8_t>(wr | 1), true);          // R/W bit 1 = read
    for (int i = 0; i < n; ++i) {
        out.push_back(c.read_byte(i + 1 < n));
    }
    c.stop();
    return true;
}

// Print the capture as two rows of characters per 64 samples: '-' high, '_' low.
inline void print_capture(const std::vector<Sample>& tr)
{
    for (std::size_t base = 0; base < tr.size(); base += 64) {
        std::string scl, sda;
        for (std::size_t i = base; i < tr.size() && i < base + 64; ++i) {
            scl += tr[i].scl ? '-' : '_';
            sda += tr[i].sda ? '-' : '_';
        }
        std::printf("%4zu SCL %s\n     SDA %s\n", base, scl.c_str(), sda.c_str());
    }
}

// The decoder: works only from the samples, like a logic analyser.
inline void decode(const std::vector<Sample>& tr)
{
    std::vector<int> bits;
    bool first_after_start = false;
    auto flush = [&]() {
        if (bits.size() == 9) {
            int v = 0;
            for (int i = 0; i < 8; ++i) v = (v << 1) | bits[i];
            const char* ack = bits[8] == 0 ? "ACK" : "NACK";
            if (first_after_start) {
                std::printf("  address 0x%02X %s  (byte 0x%02X)  %s\n", v >> 1,
                            (v & 1) ? "READ" : "WRITE", v, ack);
            } else {
                std::printf("  data    0x%02X  %s\n", v, ack);
            }
            first_after_start = false;
        }
        bits.clear();
    };
    for (std::size_t i = 1; i < tr.size(); ++i) {
        const Sample a = tr[i - 1], b = tr[i];
        if (a.scl && b.scl && a.sda && !b.sda) {
            flush();
            std::printf("  START\n");
            first_after_start = true;
        } else if (a.scl && b.scl && !a.sda && b.sda) {
            flush();
            std::printf("  STOP\n");
        } else if (!a.scl && b.scl) {          // rising SCL edge: sample SDA
            bits.push_back(b.sda);
            if (bits.size() == 9) flush();
        }
    }
}
