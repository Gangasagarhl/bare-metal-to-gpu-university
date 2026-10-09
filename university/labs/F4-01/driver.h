// driver.h - DR301: a small driver model (devices, drivers, binding, probe and remove).
// The same three objects appear in Linux (struct device, struct device_driver, bus match)
// and in Windows (device objects, driver objects, Plug and Play); see F4-01 Layer 3.
#pragma once
#include <stdint.h>
#include <stddef.h>

enum class BusKind : uint8_t { Platform, Pci };

struct Driver;

struct Device {
    const char* name;            // "com1", or "00:1f.2" for PCI
    BusKind bus;
    // identity used for matching
    const char* compatible;      // platform devices: the kind of device expected here
    uint16_t vendor, device;     // PCI IDs (0xFFFF = none)
    uint8_t cls, subcls, progif; // PCI class code
    // resources the bus discovered (one I/O range and one memory range are enough here)
    uint16_t io_base, io_len;
    uint64_t mmio_base, mmio_len;
    uint8_t irq;                 // legacy interrupt line, 0xFF = none
    // state owned by the model
    Driver* driver;              // bound driver or nullptr
    void* priv;                  // the driver's per-device state
    int last_probe;              // result of the last probe (0 = success, NOT_TRIED = none)
};

struct MatchId {                 // one row of a driver's match table
    BusKind bus;
    const char* compatible;      // platform: exact name; nullptr for PCI rows
    uint16_t vendor, device;     // PCI: 0xFFFF = any
    uint32_t class_code;         // PCI: (class << 16) | (subclass << 8) | progif
    uint32_t class_mask;         // which bits of class_code must match (0 = ignore class)
};

struct Driver {
    const char* name;
    const MatchId* ids;
    size_t n_ids;
    int (*probe)(Device&);       // 0: this driver now owns the device; negative: refused
    void (*remove)(Device&);     // release everything probe acquired
};

namespace dm {
Device& add_device(const Device& d);       // the bus reports a device
void register_driver(Driver& drv);         // a driver announces what it can handle
void bind_all();                           // try every unbound device against every driver
void unbind(Device& d);                    // remove: the driver lets go of the device
void print_table();
size_t device_count();
Device& device_at(size_t i);
bool matches(const Driver& drv, const Device& d);
}

// Error numbers returned as negative values, after the POSIX names (values are ours).
enum : int { E_NODEV = 19, E_IO = 5, E_BUSY = 16, E_TIMEDOUT = 110, E_INVAL = 22 };
constexpr int NOT_TRIED = 1;     // no driver's match table matched this device
