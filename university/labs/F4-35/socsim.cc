// socsim.cc - F4-35: a simulated SoC and a small devicetree-driven driver model on the host.
//
//   socsim <file.dtb>     probe every node of the tree, then print a register dump
//
// The SoC model is this course's own: a clock controller (one gate bit per clock), a reset
// controller (one bit per line; 1 = held in reset; everything starts in reset), a pin
// multiplexer, and a temperature sensor wired to clock 2, reset 2 and pin 4. Its rule for a
// device whose clock gate is closed: every register reads as 0 and writes are lost. That is
// ONE behaviour real SoCs show; others hang the bus or raise an external abort. Which one your
// SoC has is written (if anywhere) in its TRM - check it.
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

#include "../F4-31/fdt.h"

namespace {

// ------------------------------------------------------------------ the simulated hardware
struct Soc {
    uint32_t gates = 0;                 // CCU 0x00: bit n = clock n running
    uint32_t resets = 0xffffffff;       // RCU 0x00: bit n = reset line n asserted
    uint32_t pinfunc[8] = {};           // PINMUX 0x00 + 4n: function of pin n
    uint32_t ts_ctrl = 0;               // TSENS 0x04: bit 0 = conversion enabled
    // Hardware wiring of the sensor (fixed in silicon; software learns it from the tree).
    static constexpr int kTsClock = 2;
    static constexpr int kTsReset = 2;
    static constexpr int kTsPin = 4;
    static constexpr uint32_t kTsPinFunction = 3;

    bool ts_clocked() const { return (gates >> kTsClock & 1u) != 0; }
    bool ts_in_reset() const { return (resets >> kTsReset & 1u) != 0; }

    uint32_t read(uint32_t a) const
    {
        if (a >= 0x10000000 && a < 0x10000020) {
            return pinfunc[(a - 0x10000000) / 4];
        }
        if (a == 0x10001000) {
            return gates;
        }
        if (a == 0x10002000) {
            return resets;
        }
        if (a >= 0x10003000 && a < 0x10003100) {
            if (!ts_clocked()) {
                return 0;                                   // gated: the block does not answer
            }
            switch (a - 0x10003000) {
            case 0x00: return 0x54534e31;                   // ID: "TSN1"
            case 0x04: return ts_ctrl;
            case 0x08:                                      // DATA: milli-degrees Celsius
                if (ts_in_reset() || (ts_ctrl & 1u) == 0) {
                    return 0;
                }
                return pinfunc[kTsPin] == kTsPinFunction ? 41250 : 0x7fffffff;   // no sensor pad: rails
            default: return 0;
            }
        }
        return 0xdeadbeef;                                  // nothing decodes this address
    }

    void write(uint32_t a, uint32_t v)
    {
        if (a >= 0x10000000 && a < 0x10000020) {
            pinfunc[(a - 0x10000000) / 4] = v & 7;
        } else if (a == 0x10001000) {
            gates = v;
        } else if (a == 0x10002000) {
            resets = v;
        } else if (a == 0x10003004 && ts_clocked() && !ts_in_reset()) {
            ts_ctrl = v;
        }
    }
};

Soc g_soc;

// ------------------------------------------------------------------ the driver model
enum class Probe { kOk, kDefer, kFail };

struct Device {
    int node;
    std::string path;
    uint32_t base = 0;
    std::string state = "no driver";
    std::string why;
};

struct Ctx {
    fdt::Tree t;
    std::map<int, std::string> providers;   // node -> "clock", "reset", "pins"
};

// Resolves entry 0 of a (phandle, cells...) property; the provider must have been probed.
Probe provider_arg(Ctx& c, Device& d, const char* prop, const char* kind, std::vector<uint32_t>& args)
{
    fdt::Prop p;
    if (!c.t.prop(d.node, prop, p) || p.cells() < 1) {
        d.why = std::string("no ") + prop + " property";
        return Probe::kFail;
    }
    int prov = c.t.find_phandle(p.u32(0));
    if (prov == fdt::kNone) {
        d.why = std::string(prop) + ": phandle points nowhere";
        return Probe::kFail;
    }
    if (c.providers.count(prov) == 0 || c.providers[prov] != kind) {
        d.why = std::string(kind) + " provider not probed yet";
        return Probe::kDefer;
    }
    args.assign(p.cells() - 1, 0);
    for (uint32_t i = 1; i < p.cells(); ++i) {
        args[i - 1] = p.u32(i);
    }
    return Probe::kOk;
}

Probe ccu_probe(Ctx& c, Device& d)
{
    c.providers[d.node] = "clock";
    std::printf("  ccu    %s: clock provider, #clock-cells = %u\n", d.path.c_str(), c.t.u32(d.node, "#clock-cells", 0));
    return Probe::kOk;
}

Probe rcu_probe(Ctx& c, Device& d)
{
    c.providers[d.node] = "reset";
    std::printf("  rcu    %s: reset provider, lines held: 0x%08x\n", d.path.c_str(), g_soc.read(d.base));
    return Probe::kOk;
}

Probe pinmux_probe(Ctx& c, Device& d)
{
    c.providers[d.node] = "pins";
    std::printf("  pinmux %s: pin provider\n", d.path.c_str());
    return Probe::kOk;
}

// The sensor driver: clock on, reset off, pin function, start conversions, read once.
Probe tsens_probe(Ctx& c, Device& d)
{
    std::vector<uint32_t> clk, rst, pin;
    for (auto [prop, kind, out] : {std::tuple{"clocks", "clock", &clk}, std::tuple{"resets", "reset", &rst},
                                   std::tuple{"pins", "pins", &pin}}) {
        Probe r = provider_arg(c, d, prop, kind, *out);
        if (r != Probe::kOk) {
            return r;
        }
    }
    if (clk.size() != 1 || rst.size() != 1 || pin.size() != 3) {
        d.why = "unexpected number of specifier cells";
        return Probe::kFail;
    }
    g_soc.write(0x10001000, g_soc.read(0x10001000) | 1u << clk[0]);         // ungate clock clk[0]
    g_soc.write(0x10002000, g_soc.read(0x10002000) & ~(1u << rst[0]));      // release reset rst[0]
    for (uint32_t i = 0; i < pin[1]; ++i) {
        g_soc.write(0x10000000 + 4 * (pin[0] + i), pin[2]);                // pin function
    }
    g_soc.write(d.base + 0x04, 1);                                           // start conversions
    uint32_t raw = g_soc.read(d.base + 0x08);
    std::printf("  tsens  %s: clock %u on, reset %u released, pin %u function %u; temperature %.3f C\n",
                d.path.c_str(), clk[0], rst[0], pin[0], pin[2], static_cast<int32_t>(raw) / 1000.0);
    return Probe::kOk;
}

struct Driver {
    const char* compatible;
    Probe (*probe)(Ctx&, Device&);
};
const Driver kDrivers[] = {
    {"dr403,ccu", ccu_probe}, {"dr403,rcu", rcu_probe}, {"dr403,pinmux", pinmux_probe}, {"dr403,tsens", tsens_probe},
};

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: socsim file.dtb\n");
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> blob((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    Ctx c;
    if (blob.size() < 40 || !c.t.init(blob.data())) {
        std::fprintf(stderr, "socsim: %s is not a devicetree blob\n", argv[1]);
        return 1;
    }
    std::vector<Device> devs;
    for (int n = c.t.root(); n != fdt::kNone; n = c.t.next_node(n)) {
        fdt::Prop p;
        if (!c.t.prop(n, "compatible", p)) {
            continue;
        }
        char path[96];
        c.t.path(n, path, sizeof path);
        Device d{n, path, 0, "no driver", ""};
        uint64_t a = 0;
        uint64_t s = 0;
        if (c.t.mmio(n, 0, a, s)) {
            d.base = static_cast<uint32_t>(a);
        }
        if (!c.t.available(n)) {
            d.state = "disabled";
        } else {
            for (const Driver& drv : kDrivers) {
                if (c.t.compatible(n, drv.compatible)) {
                    d.state = "waiting";
                }
            }
        }
        devs.push_back(d);
    }
    for (int pass = 1;; ++pass) {
        int done = 0;
        int waiting = 0;
        for (Device& d : devs) {
            if (d.state != "waiting") {
                continue;
            }
            for (const Driver& drv : kDrivers) {
                if (!c.t.compatible(d.node, drv.compatible)) {
                    continue;
                }
                Probe r = drv.probe(c, d);
                if (r == Probe::kDefer) {
                    ++waiting;
                    std::printf("  pass %d: %s deferred (%s)\n", pass, d.path.c_str(), d.why.c_str());
                } else {
                    d.state = r == Probe::kOk ? "bound" : "FAILED";
                    ++done;
                }
            }
        }
        std::printf("probe pass %d: %d finished, %d still waiting\n", pass, done, waiting);
        if (waiting == 0 || done == 0) {
            break;
        }
    }
    for (const Device& d : devs) {
        std::printf("  %-9s %s%s%s\n", d.state.c_str(), d.path.c_str(), d.why.empty() || d.state == "bound" ? "" : ": ",
                    d.state == "bound" ? "" : d.why.c_str());
    }
    std::printf("register dump (address: value)\n");
    const std::pair<uint32_t, const char*> regs[] = {
        {0x10001000, "CCU GATES"}, {0x10002000, "RCU ASSERT"}, {0x10000010, "PINMUX PIN4"},
        {0x10003000, "TSENS ID"}, {0x10003004, "TSENS CTRL"}, {0x10003008, "TSENS DATA"},
    };
    for (auto [addr, name] : regs) {
        std::printf("  0x%08x: 0x%08x  %s\n", addr, g_soc.read(addr), name);
    }
    return 0;
}
