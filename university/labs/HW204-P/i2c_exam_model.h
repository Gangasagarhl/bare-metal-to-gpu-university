// HW204 practical (P): the course's own I2C bus model (the same idea as F1-47's
// i2c_sim.h, which stands for a real I2C controller plus a sigrok/PulseView
// logic-analyser capture), extended so that several targets can sit on the bus.
// Both lines are open-drain: a line is low if ANY side pulls it low, high
// (through the pull-up) otherwise. Each bit is four samples (SCL low, low,
// high, high). All addresses and register values are EXERCISE values, not a
// real part's (owner ruling A4).
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

struct Target                             // one chip on the bus
{
    std::uint8_t address;                 // 7-bit address
    std::uint8_t regs[4];                 // four registers; the pointer wraps modulo 4
    std::uint8_t pointer = 0;
    bool selected = false;
    bool first_byte_after_address = true;
};

class Bus
{
public:
    std::vector<Sample> trace;
    std::vector<Target*> targets;
    int scl = 1;                          // controller's SCL output (1 = released)
    int sda_ctrl = 1;                     // controller's SDA output
    int sda_target = 1;                   // wired-AND of all targets' SDA outputs
    void tick() { trace.push_back({scl, sda_ctrl & sda_target}); }
};

class Controller
{
public:
    Bus& bus;
    explicit Controller(Bus& b) : bus(b) {}

    void start()
    {
        if (bus.scl == 0) {               // repeated START: release SDA while SCL
            bus.sda_ctrl = 1; bus.tick(); // is low, then raise SCL
            bus.scl = 1; bus.tick();
        } else {
            bus.sda_ctrl = 1; bus.tick(); // bus idle: both lines high
        }
        bus.sda_ctrl = 0; bus.tick();     // SDA falls while SCL is high
        bus.scl = 0; bus.tick();
    }
    void stop()
    {
        bus.sda_ctrl = 0; bus.scl = 0; bus.tick();
        bus.scl = 1; bus.tick();
        bus.sda_ctrl = 1; bus.tick();     // SDA rises while SCL is high
        bus.tick();
        for (Target* t : bus.targets) t->selected = false;
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
        bus.sda_ctrl = 1;                 // release SDA for the ACK bit
        bool ack = false;
        for (Target* t : bus.targets) {
            if (is_address) {
                t->selected = (b >> 1) == t->address;   // the R/W bit is not part of the address
                t->first_byte_after_address = true;
                if (t->selected) ack = true;
            } else if (t->selected) {
                if (t->first_byte_after_address) {
                    t->pointer = b & 3;   // the first data byte sets the register pointer
                    t->first_byte_after_address = false;
                } else {
                    t->regs[t->pointer] = b;             // later bytes write registers
                    t->pointer = static_cast<std::uint8_t>((t->pointer + 1) & 3);
                }
                ack = true;
            }
        }
        bus.sda_target = ack ? 0 : 1;
        clock_bit();
        bus.sda_target = 1;
        return ack;
    }
    std::uint8_t read_byte(bool ack)
    {
        Target* sel = nullptr;
        for (Target* t : bus.targets) if (t->selected) sel = t;
        const std::uint8_t out = sel ? sel->regs[sel->pointer] : 0xFF;   // nobody: line stays high
        std::uint8_t v = 0;
        bus.sda_ctrl = 1;
        for (int i = 7; i >= 0; --i) {
            bus.sda_target = (out >> i) & 1;             // the target drives SDA
            clock_bit();
            v = static_cast<std::uint8_t>((v << 1) | bus.trace.back().sda);
        }
        bus.sda_target = 1;
        if (sel) sel->pointer = static_cast<std::uint8_t>((sel->pointer + 1) & 3);
        bus.sda_ctrl = ack ? 0 : 1;       // the controller ACKs all but the last byte
        clock_bit();
        bus.sda_ctrl = 1;
        return v;
    }
};

// Address probe: START, address + W, STOP. Returns true on ACK.
inline bool probe(Controller& c, unsigned addr7)
{
    c.start();
    const bool ack = c.write_byte(static_cast<std::uint8_t>(addr7 << 1), true);
    c.stop();
    return ack;
}

// Write one register: START, address + W, register, value, STOP.
inline bool write_reg(Controller& c, unsigned addr7, std::uint8_t reg, std::uint8_t value)
{
    c.start();
    if (!c.write_byte(static_cast<std::uint8_t>(addr7 << 1), true)) { c.stop(); return false; }
    c.write_byte(reg, false);
    c.write_byte(value, false);
    c.stop();
    return true;
}

// Read n bytes from a register: address + W, register, repeated START, address + R, data, STOP.
inline bool read_reg(Controller& c, unsigned addr7, std::uint8_t reg, int n,
                     std::vector<std::uint8_t>& out)
{
    c.start();
    const auto wr = static_cast<std::uint8_t>(addr7 << 1);
    if (!c.write_byte(wr, true)) { c.stop(); return false; }
    c.write_byte(reg, false);
    c.start();                            // repeated START
    c.write_byte(static_cast<std::uint8_t>(wr | 1), true);
    for (int i = 0; i < n; ++i) out.push_back(c.read_byte(i + 1 < n));
    c.stop();
    return true;
}

// Print the capture as two rows of characters per 64 samples: '-' high, '_' low
// (the same text format as F1-47's capture).
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
