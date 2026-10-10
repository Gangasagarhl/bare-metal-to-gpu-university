// i2c_hid_sim.cc - a laptop touchpad on an I2C bus, as a model, and a driver for it.
// The bus transfer has the shape of Linux's I2C_RDWR: an array of struct i2c_msg (from
// <linux/i2c.h>), each a START + address + data, a read when I2C_M_RD is set; several messages
// in one transfer are joined by repeated STARTs. A message to an address with no device answers
// NACK (-ENXIO). The touchpad model speaks the HID-over-I2C register protocol as this course
// remembers it (register numbers, the 30-byte HID descriptor layout and the command opcodes are
// from memory of Microsoft's "HID over I2C Protocol Specification", title only: see the
// chapter's unverified box). Its interrupt is a level-triggered GPIO line: asserted while the
// device holds an input report, released when the host has read it.
// Usage: i2c_hid_sim <descriptors file from hid_extract.py>
#include "hid_parse.h"
#include <linux/i2c.h>
#include <cerrno>
#include <cstdio>
#include <deque>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------- the bus
struct I2cDevice {
    virtual ~I2cDevice() = default;
    virtual void write(const uint8_t* b, uint16_t n) = 0;
    virtual void read(uint8_t* b, uint16_t n) = 0;
};
struct Bus {
    std::map<uint16_t, I2cDevice*> dev;
    bool trace = false;
    int transfer(i2c_msg* m, int n)
    {
        for (int i = 0; i < n; ++i) {
            auto it = dev.find(m[i].addr);
            if (trace) std::printf("    i2c %s 0x%02x %s len %u", i ? "Sr" : "S ", m[i].addr,
                                   (m[i].flags & I2C_M_RD) ? "R" : "W", m[i].len);
            if (it == dev.end()) { if (trace) std::printf(" -> NACK\n"); return -ENXIO; }
            if (m[i].flags & I2C_M_RD) it->second->read(m[i].buf, m[i].len);
            else it->second->write(m[i].buf, m[i].len);
            if (trace) {
                std::printf(" :");
                for (int k = 0; k < m[i].len && k < 12; ++k) std::printf(" %02x", m[i].buf[k]);
                std::printf(m[i].len > 12 ? " ...\n" : "\n");
            }
        }
        if (trace) std::printf("    i2c P\n");
        return n;
    }
};

// ---------------------------------------------------------------- the touchpad model
// Model registers (16-bit, little endian on the wire).
constexpr uint16_t kRegHidDesc = 0x0001, kRegReportDesc = 0x0002, kRegInput = 0x0003,
                   kRegOutput = 0x0004, kRegCommand = 0x0005, kRegData = 0x0006;
constexpr uint8_t kOpReset = 0x01, kOpSetPower = 0x08;

struct Touchpad : I2cDevice {
    std::vector<uint8_t> report_desc;
    std::deque<std::vector<uint8_t>> pending;     // input reports waiting for the host
    uint16_t reg = 0;
    bool powered = false;
    bool irq_line() const { return !pending.empty(); }

    std::vector<uint8_t> hid_descriptor() const
    {
        const uint16_t w[15] = {30, 0x0100, static_cast<uint16_t>(report_desc.size()), kRegReportDesc,
                                kRegInput, 2 + 6, kRegOutput, 0, kRegCommand, kRegData,
                                0x1234, 0x5678, 0x0001, 0, 0};
        std::vector<uint8_t> d;
        for (uint16_t v : w) { d.push_back(v & 0xFF); d.push_back(v >> 8); }
        return d;                                   // 30 bytes
    }
    void write(const uint8_t* b, uint16_t n) override
    {
        if (n < 2) return;
        reg = static_cast<uint16_t>(b[0] | (b[1] << 8));
        if (reg == kRegCommand && n >= 4) {
            const uint8_t op = b[3] & 0x0F;
            if (op == kOpReset) { pending.clear(); powered = true; pending.push_back({0x00, 0x00}); }
            if (op == kOpSetPower) powered = (b[2] & 0x03) == 0;   // 0 = on, 1 = sleep
        }
    }
    void read(uint8_t* b, uint16_t n) override
    {
        std::vector<uint8_t> src;
        if (reg == kRegHidDesc) src = hid_descriptor();
        else if (reg == kRegReportDesc) src = report_desc;
        else if (reg == kRegInput || reg == kRegCommand) {     // a read without a register write
            if (!pending.empty()) { src = pending.front(); pending.pop_front(); }
            else src = {0x00, 0x00};
        }
        for (uint16_t i = 0; i < n; ++i) b[i] = i < src.size() ? src[i] : 0;
        reg = kRegInput;                            // after any read, a bare read is an input read
    }
    // A finger at (x, y) with the given buttons: queue one input report (2 length bytes first).
    void touch(uint16_t x, uint16_t y, uint8_t buttons)
    {
        if (!powered) return;
        pending.push_back({8, 0, buttons, static_cast<uint8_t>(x), static_cast<uint8_t>(x >> 8),
                           static_cast<uint8_t>(y), static_cast<uint8_t>(y >> 8), 0});
    }
};

// ---------------------------------------------------------------- the driver
struct Driver {
    Bus& bus;
    uint16_t addr = 0;
    uint16_t input_reg = 0, max_input = 0, command_reg = 0;
    hid::Field fx{}, fy{}, fbtn{};
    int reports = 0;
    std::vector<std::vector<int>> events;          // decoded x, y, buttons

    explicit Driver(Bus& b) : bus(b) {}
    // Write a register number, then read len bytes: one transfer, repeated START between.
    int read_reg(uint16_t a, uint16_t reg, std::vector<uint8_t>& out, uint16_t len)
    {
        uint8_t r[2] = {static_cast<uint8_t>(reg), static_cast<uint8_t>(reg >> 8)};
        out.assign(len, 0);
        i2c_msg m[2] = {{a, 0, 2, r}, {a, I2C_M_RD, len, out.data()}};
        return bus.transfer(m, 2);
    }
    int command(uint8_t low, uint8_t op)
    {
        uint8_t b[4] = {static_cast<uint8_t>(command_reg), static_cast<uint8_t>(command_reg >> 8), low, op};
        i2c_msg m = {addr, 0, 4, b};
        return bus.transfer(&m, 1);
    }
    int read_input(std::vector<uint8_t>& out)
    {
        out.assign(max_input, 0);
        i2c_msg m = {addr, I2C_M_RD, max_input, out.data()};
        return bus.transfer(&m, 1);
    }
    static uint16_t w16(const std::vector<uint8_t>& d, int i) { return static_cast<uint16_t>(d[i] | (d[i + 1] << 8)); }

    bool probe(uint16_t a, uint16_t desc_reg)
    {
        std::vector<uint8_t> d;
        std::printf("probe 0x%02x: read HID descriptor (register 0x%04x)\n", a, desc_reg);
        const int rc = read_reg(a, desc_reg, d, 30);
        if (rc < 0) { std::printf("  no answer (%s): nothing at this address\n", rc == -ENXIO ? "NACK" : "error"); return false; }
        if (w16(d, 0) != 30 || w16(d, 2) != 0x0100) {
            std::printf("  descriptor length %u version 0x%04x: not a HID-over-I2C device as expected\n", w16(d, 0), w16(d, 2));
            return false;
        }
        addr = a;
        const uint16_t rlen = w16(d, 4), rreg = w16(d, 6);
        input_reg = w16(d, 8); max_input = w16(d, 10); command_reg = w16(d, 16);
        std::printf("  descriptor: report descriptor %u bytes at 0x%04x, input 0x%04x max %u, command 0x%04x, vendor %04x product %04x\n",
                    rlen, rreg, input_reg, max_input, command_reg, w16(d, 20), w16(d, 22));
        std::vector<uint8_t> rd;
        if (read_reg(a, rreg, rd, rlen) < 0) return false;
        const hid::Parsed p = hid::parse(rd);
        if (!p.error.empty()) { std::printf("  report descriptor: %s\n", p.error.c_str()); return false; }
        bool hx = false, hy = false, hb = false;
        for (const auto& f : p.fields) {
            if (f.flags & 1) continue;
            for (uint32_t u = f.usage, i = 0; u <= f.usage_max; ++u, ++i) {
                hid::Field one = f;
                one.bit_offset = f.bit_offset + i * f.size; one.count = 1; one.usage = one.usage_max = u;
                if (f.usage_page == 0x01 && u == 0x30) { fx = one; hx = true; }
                if (f.usage_page == 0x01 && u == 0x31) { fy = one; hy = true; }
                if (f.usage_page == 0x09 && u == 0x01) { fbtn = one; hb = true; }
                if (u == f.usage_max) break;
            }
        }
        std::printf("  report descriptor parsed: X bits %u+%u, Y bits %u+%u, button 1 bit %u\n",
                    fx.bit_offset, fx.size, fy.bit_offset, fy.size, fbtn.bit_offset);
        if (!(hx && hy && hb)) { std::printf("  no X, Y and button 1: not a pointer\n"); return false; }
        return true;
    }
    // The interrupt handler: read the input register; a length of 0 is "nothing / reset done".
    void irq()
    {
#ifdef F421_NO_READ
        return;                                     // forensic version: acknowledges nothing
#else
        std::vector<uint8_t> r;
        if (read_input(r) < 0) return;
        const uint16_t len = w16(r, 0);
        if (len < 2) return;
        ++reports;
        const std::vector<uint8_t> body(r.begin() + 2, r.begin() + len);
        events.push_back({hid::extract(body, fx, 0), hid::extract(body, fy, 0), hid::extract(body, fbtn, 0)});
#endif
    }
};

static std::vector<uint8_t> tablet_descriptor(const char* path)
{
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream s(line);
        std::string off, usage, len, hex;
        s >> off >> usage >> len >> hex;
        std::vector<uint8_t> d;
        for (size_t i = 0; i + 1 < hex.size(); i += 2) d.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
        const hid::Parsed p = hid::parse(d);
        for (const auto& f : p.fields)       // the absolute pointer: X with a 16-bit field, no report ID
            if (f.usage_page == 1 && f.usage == 0x30 && f.size == 16 && !(f.flags & 4) && f.report_id == 0) {
                std::printf("touchpad model uses the descriptor at file offset %s (%zu bytes)\n", off.c_str(), d.size());
                return d;
            }
    }
    return {};
}

int main(int argc, char** argv)
{
    if (argc < 2) { std::fprintf(stderr, "usage: i2c_hid_sim descriptors.txt\n"); return 2; }
    Touchpad pad;
    pad.report_desc = tablet_descriptor(argv[1]);
    if (pad.report_desc.empty()) { std::printf("no absolute-pointer descriptor found\n"); return 2; }
    Bus bus;
    bus.dev[0x2c] = &pad;                           // the model's address (a board's ACPI table gives the real one)
    bus.trace = true;
    Driver drv(bus);

    std::printf("== probe ==\n");
    drv.probe(0x2d, kRegHidDesc);                   // a wrong address first: NACK
    if (!drv.probe(0x2c, kRegHidDesc)) return 1;
    std::printf("== reset and power on ==\n");
    drv.command(0x00, kOpReset);
    std::printf("  interrupt line after RESET: %s\n", pad.irq_line() ? "asserted" : "released");
    std::vector<uint8_t> r;
    drv.read_input(r);
    std::printf("  reset answer: length %u; line now %s\n", Driver::w16(r, 0), pad.irq_line() ? "asserted" : "released");
    drv.command(0x00, kOpSetPower);                 // power state 0 = on
    bus.trace = false;

    // 200 ticks of the GPIO controller: level-triggered, so the handler runs on every tick the
    // line is asserted. A finger moves during ticks 20..24, clicks at tick 100, then nothing.
    std::printf("== 200 ticks ==\n");
    struct Touch { int tick; uint16_t x, y; uint8_t b; };
    const std::vector<Touch> script = {{20, 1000, 2000, 0}, {21, 1100, 2050, 0}, {22, 1200, 2100, 0},
                                       {23, 1300, 2150, 0}, {24, 1400, 2200, 0}, {100, 1400, 2200, 1},
                                       {101, 1400, 2200, 0}};
    int interrupts = 0, idle_interrupts = 0;
    size_t next = 0;
    for (int tick = 0; tick < 200; ++tick) {
        bool touched = false;
        while (next < script.size() && script[next].tick == tick) {
            pad.touch(script[next].x, script[next].y, script[next].b);
            ++next; touched = true;
        }
        if (pad.irq_line()) {
            ++interrupts;
            if (!touched) ++idle_interrupts;
            drv.irq();
        }
    }
    std::printf("interrupts %d (of them on ticks with no touch: %d), reports read %d\n", interrupts, idle_interrupts, drv.reports);
    int wrong = 0;
    for (size_t i = 0; i < drv.events.size() && i < script.size(); ++i) {
        const auto& e = drv.events[i];
        const bool ok = e[0] == script[i].x && e[1] == script[i].y && e[2] == script[i].b;
        if (!ok) ++wrong;
        std::printf("  event %zu: x %5d y %5d button %d %s\n", i, e[0], e[1], e[2], ok ? "matches the touch" : "WRONG");
    }
    const bool pass = drv.events.size() == script.size() && wrong == 0 && idle_interrupts == 0;
    std::printf("%s\n", pass ? "PASS: every touch decoded, no interrupt without a touch"
                             : "FAIL: see the counts above");
    return pass ? 0 : 1;
}
