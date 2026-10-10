// gtimer.cc - F4-25: find the timer interrupt in the devicetree.
#include "fdt.h"
#include "gtimer.h"

namespace gtimer {

uint32_t intid_from_devicetree(const fdt::Blob& dt)
{
    fdt::Node t;
    fdt::Prop p;
    // interrupts = <type number flags> x 4: secure phys, non-secure phys, virtual, hypervisor
    if (!dt.find_compatible("arm,armv8-timer", &t) || !dt.get_prop(t, "interrupts", &p) || p.len < 24) {
        return 30;   // not reached on QEMU virt; a board without the node needs its own answer
    }
    return 16 + fdt::be32(p.data + 12 + 4);   // entry 1, cell 1: the PPI number
}

} // namespace gtimer
