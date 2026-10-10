// driver.cc - DR301: the driver model. Fixed-size tables, no heap.
#include "driver.h"
#include "kbase.h"

namespace {
constexpr size_t MAX_DEVICES = 48;
constexpr size_t MAX_DRIVERS = 16;
Device g_devices[MAX_DEVICES];
size_t g_ndevices = 0;
Driver* g_drivers[MAX_DRIVERS];
size_t g_ndrivers = 0;

bool id_matches(const MatchId& id, const Device& d)
{
    if (id.bus != d.bus) return false;
    if (d.bus == BusKind::Platform)
        return id.compatible && d.compatible && kstrcmp(id.compatible, d.compatible) == 0;
    if (id.vendor != 0xFFFF && id.vendor != d.vendor) return false;
    if (id.device != 0xFFFF && id.device != d.device) return false;
    const uint32_t cc = (uint32_t{d.cls} << 16) | (uint32_t{d.subcls} << 8) | d.progif;
    return (cc & id.class_mask) == (id.class_code & id.class_mask);
}
}  // namespace

namespace dm {
bool matches(const Driver& drv, const Device& d)
{
    for (size_t i = 0; i < drv.n_ids; ++i)
        if (id_matches(drv.ids[i], d)) return true;
    return false;
}

Device& add_device(const Device& d)
{
    if (g_ndevices == MAX_DEVICES) panic("device table full");
    Device& slot = g_devices[g_ndevices++];
    slot = d;
    slot.driver = nullptr;
    slot.priv = nullptr;
    slot.last_probe = NOT_TRIED;
    return slot;
}

void register_driver(Driver& drv)
{
    if (g_ndrivers == MAX_DRIVERS) panic("driver table full");
    g_drivers[g_ndrivers++] = &drv;
}

void bind_all()
{
    for (size_t i = 0; i < g_ndevices; ++i) {
        Device& d = g_devices[i];
        if (d.driver) continue;
        for (size_t k = 0; k < g_ndrivers; ++k) {
            Driver& drv = *g_drivers[k];
            if (!matches(drv, d)) continue;
            int rc = drv.probe(d);
            d.last_probe = rc;
            if (rc == 0) { d.driver = &drv; break; }   // first driver that accepts wins
        }
    }
}

void unbind(Device& d)
{
    if (!d.driver) return;
    if (d.driver->remove) d.driver->remove(d);
    d.driver = nullptr;
    d.priv = nullptr;
}

size_t device_count() { return g_ndevices; }
Device& device_at(size_t i) { return g_devices[i]; }

void print_table()
{
    kprintf("%-8s %-9s %-13s %-12s %s\n", "device", "bus", "I/O ports", "driver", "probe");
    for (size_t i = 0; i < g_ndevices; ++i) {
        const Device& d = g_devices[i];
        kprintf("%-8s %-9s ", d.name, d.bus == BusKind::Pci ? "pci" : "platform");
        if (d.io_len) kprintf("0x%04x-0x%04x ", d.io_base, d.io_base + d.io_len - 1);
        else kprintf("%-13s ", "-");
        kprintf("%-12s ", d.driver ? d.driver->name : "(none)");
        if (d.last_probe == NOT_TRIED) kprintf("no matching driver\n");
        else kprintf("%d\n", d.last_probe);
    }
}
}  // namespace dm
