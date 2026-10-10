// dm.h - F4-35: the DR403 driver model. Devices come from the devicetree; drivers declare the
// compatible strings they handle; probing is retried while a provider (clock, interrupt
// controller, pins) that a device needs has not been probed yet ("deferred probe").
#pragma once
#include "../F4-31/fdt.h"
#include "../F4-31/kbase.h"

namespace dm {

enum class Probe { kOk, kDefer, kFail };
enum class State { kNoDriver, kWaiting, kBound, kFailed, kDisabled };

struct Device {
    int node = fdt::kNone;
    const struct Driver* driver = nullptr;
    State state = State::kNoDriver;
    uint64_t base = 0;          // first "reg" entry, translated to a CPU address (0 if none)
    uint64_t size = 0;
    const char* why = "";       // last reason given by probe (deferral or failure)
    int defers = 0;
};

struct Driver {
    const char* name;
    const char* const* compatible;  // nullptr-terminated list
    Probe (*probe)(Device& dev);
};

// Each driver file adds one descriptor to the linker section "dr403_drivers".
#define DR403_DRIVER(id, drvname, probe_fn, ...)                                                   \
    static const char* const id##_compat[] = {__VA_ARGS__, nullptr};                              \
    __attribute__((used, section("dr403_drivers"))) static const dm::Driver id##_driver = {        \
        drvname, id##_compat, probe_fn}

const fdt::Tree& tree();
void init(const void* dtb);           // builds the device list from the tree
void probe_all();                      // probes in passes until nothing changes
void report();                         // prints bound, failed, waiting and driverless nodes
int count(State s);
const char* path(const Device& dev);   // devicetree path of a device (static buffer)

// --- providers ---------------------------------------------------------------------------
// Clocks: a provider registers itself with a function that turns the specifier cells of a
// consumer's "clocks" entry into a rate. A consumer asks for entry `index` of its "clocks".
using ClockRateFn = bool (*)(int provider_node, const uint32_t* cells, uint32_t ncells, uint64_t& rate,
                             bool enable);
void clock_provider(int node, ClockRateFn fn);
Probe clock_enable(Device& dev, int index, uint64_t& rate);   // kDefer if the provider is missing

// Interrupt controllers: a device's interrupt parent must have been probed before it.
void irq_provider(int node);
Probe irq_parent_ready(Device& dev);

// Pin control: a provider sets the function of a group of pins on request.
using PinFn = bool (*)(int provider_node, uint32_t first_pin, uint32_t count, uint32_t function);
void pin_provider(int node, PinFn fn);
Probe pins_set(Device& dev, const char* property);           // property = <&provider first count function>

// The power-off hook (set by a PSCI driver if the tree has one).
void set_power_off(void (*fn)());
[[noreturn]] void power_off(int code_if_none);

}  // namespace dm
