// dm.cc - F4-35: the driver-model core (see dm.h).
#include "dm.h"

extern "C" const dm::Driver __drivers_start[];
extern "C" const dm::Driver __drivers_end[];

namespace dm {
namespace {
constexpr int kMaxDevices = 128;
constexpr int kMaxProviders = 16;
fdt::Tree g_tree;
Device g_dev[kMaxDevices];
int g_ndev = 0;
struct ClockProv { int node; ClockRateFn fn; };
ClockProv g_clk[kMaxProviders];
int g_nclk = 0;
int g_irq[kMaxProviders];
int g_nirq = 0;
struct PinProv { int node; PinFn fn; };
PinProv g_pin[kMaxProviders];
int g_npin = 0;
void (*g_power_off)() = nullptr;

const Driver* match(int node)
{
    for (const Driver* d = __drivers_start; d < __drivers_end; ++d) {
        for (const char* const* c = d->compatible; *c != nullptr; ++c) {
            if (g_tree.compatible(node, *c)) {
                return d;
            }
        }
    }
    return nullptr;
}

const char* state_name(State s)
{
    switch (s) {
    case State::kBound: return "bound";
    case State::kFailed: return "FAILED";
    case State::kWaiting: return "WAITING";
    case State::kDisabled: return "disabled";
    default: return "no driver";
    }
}
}  // namespace

const fdt::Tree& tree() { return g_tree; }

const char* path(const Device& dev)
{
    static char buf[96];
    g_tree.path(dev.node, buf, sizeof buf);
    return buf;
}

void init(const void* dtb)
{
    g_tree.init(dtb);
    fdt::Prop p;
    for (int n = g_tree.root(); n != fdt::kNone && g_ndev < kMaxDevices; n = g_tree.next_node(n)) {
        if (!g_tree.prop(n, "compatible", p)) {
            continue;
        }
        Device& d = g_dev[g_ndev++];
        d.node = n;
        d.driver = match(n);
        g_tree.mmio(n, 0, d.base, d.size);
        if (!g_tree.available(n)) {
            d.state = State::kDisabled;
        } else {
            d.state = d.driver != nullptr ? State::kWaiting : State::kNoDriver;
        }
    }
}

void probe_all()
{
    for (int pass = 1;; ++pass) {
        int progress = 0;
        int waiting = 0;
        for (int i = 0; i < g_ndev; ++i) {
            Device& d = g_dev[i];
            if (d.state != State::kWaiting) {
                continue;
            }
            Probe r = d.driver->probe(d);
            if (r == Probe::kOk) {
                d.state = State::kBound;
                ++progress;
            } else if (r == Probe::kFail) {
                d.state = State::kFailed;
                ++progress;
            } else {
                ++d.defers;
                ++waiting;
                if (waiting <= 3) {
                    k::printf("  pass %d: %s deferred (%s)\n", pass, path(d), d.why);
                } else if (waiting == 4) {
                    k::printf("  pass %d: (further deferrals counted, not listed)\n", pass);
                }
            }
        }
        k::printf("probe pass %d: %d finished, %d still waiting\n", pass, progress, waiting);
        if (waiting == 0 || progress == 0) {
            return;
        }
    }
}

int count(State s)
{
    int n = 0;
    for (int i = 0; i < g_ndev; ++i) {
        n += g_dev[i].state == s ? 1 : 0;
    }
    return n;
}

void report()
{
    k::printf("devices with a compatible: %d; bound %d, failed %d, waiting %d, disabled %d, no driver %d\n",
              g_ndev, count(State::kBound), count(State::kFailed), count(State::kWaiting),
              count(State::kDisabled), count(State::kNoDriver));
    int virtio_empty = 0;
    for (int i = 0; i < g_ndev; ++i) {
        const Device& d = g_dev[i];
        fdt::Prop c;
        g_tree.prop(d.node, "compatible", c);
        if (d.state == State::kFailed && fdt::streq(d.why, "empty transport (device ID 0)")) {
            ++virtio_empty;             // nothing is plugged into this virtio slot
            continue;
        }
        k::printf("  %-9s %-34s %-26s %s%s\n", state_name(d.state), path(d),
                  reinterpret_cast<const char*>(c.data), d.driver != nullptr ? d.driver->name : "-",
                  d.state == State::kBound && d.defers ? " (after deferral)" : "");
    }
    if (virtio_empty != 0) {
        k::printf("  (%d FAILED virtio,mmio transports not listed: empty transport, device ID 0)\n", virtio_empty);
    }
}

void clock_provider(int node, ClockRateFn fn)
{
    if (g_nclk < kMaxProviders) {
        g_clk[g_nclk++] = {node, fn};
    }
}

// "clocks" is a list of (phandle, specifier cells...) entries; the provider's #clock-cells says
// how many specifier cells follow each phandle (Devicetree Specification and the common clock
// binding - title only, pending verification).
Probe clock_enable(Device& dev, int index, uint64_t& rate)
{
    fdt::Prop p;
    if (!g_tree.prop(dev.node, "clocks", p)) {
        dev.why = "no clocks property";
        return Probe::kFail;
    }
    uint32_t cell = 0;
    for (int i = 0; cell < p.cells(); ++i) {
        int prov = g_tree.find_phandle(p.u32(cell));
        if (prov == fdt::kNone) {
            dev.why = "clocks: phandle points nowhere";
            return Probe::kFail;
        }
        uint32_t n = g_tree.u32(prov, "#clock-cells", 0);
        if (i == index) {
            uint32_t spec[4] = {0, 0, 0, 0};
            for (uint32_t k = 0; k < n && k < 4; ++k) {
                spec[k] = p.u32(cell + 1 + k);
            }
            for (int c = 0; c < g_nclk; ++c) {
                if (g_clk[c].node == prov) {
                    if (!g_clk[c].fn(prov, spec, n, rate, true)) {
                        dev.why = "clock provider refused the request";
                        return Probe::kFail;
                    }
                    return Probe::kOk;
                }
            }
            dev.why = "clock provider not probed yet";
            return Probe::kDefer;
        }
        cell += 1 + n;
    }
    dev.why = "clocks: index out of range";
    return Probe::kFail;
}

void irq_provider(int node)
{
    if (g_nirq < kMaxProviders) {
        g_irq[g_nirq++] = node;
    }
}

// The interrupt parent is the nearest "interrupt-parent" property on the node or an ancestor.
Probe irq_parent_ready(Device& dev)
{
    uint32_t ph = 0;
    for (int n = dev.node; n != fdt::kNone && ph == 0; n = g_tree.parent(n)) {
        ph = g_tree.u32(n, "interrupt-parent", 0);
    }
    int parent = ph != 0 ? g_tree.find_phandle(ph) : fdt::kNone;
    if (parent == fdt::kNone) {
        dev.why = "no interrupt parent in the tree";
        return Probe::kFail;
    }
    for (int i = 0; i < g_nirq; ++i) {
        if (g_irq[i] == parent) {
            return Probe::kOk;
        }
    }
    dev.why = "interrupt controller not probed yet";
    return Probe::kDefer;
}

void pin_provider(int node, PinFn fn)
{
    if (g_npin < kMaxProviders) {
        g_pin[g_npin++] = {node, fn};
    }
}

Probe pins_set(Device& dev, const char* property)
{
    fdt::Prop p;
    if (!g_tree.prop(dev.node, property, p) || p.cells() != 4) {
        dev.why = "pin property missing or not 4 cells";
        return Probe::kFail;
    }
    int prov = g_tree.find_phandle(p.u32(0));
    for (int i = 0; i < g_npin; ++i) {
        if (g_pin[i].node == prov) {
            if (!g_pin[i].fn(prov, p.u32(1), p.u32(2), p.u32(3))) {
                dev.why = "pin provider refused the request";
                return Probe::kFail;
            }
            return Probe::kOk;
        }
    }
    dev.why = "pin controller not probed yet";
    return Probe::kDefer;
}

void set_power_off(void (*fn)()) { g_power_off = fn; }

void power_off(int code_if_none)
{
    if (g_power_off != nullptr) {
        g_power_off();
    }
    k::exit(code_if_none);
}

}  // namespace dm
