// ahci.h - DR301 F4-06: an AHCI host bus adapter driver for SATA disks.
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace ahci {
struct DiskInfo {
    int port;
    char model[41], serial[21], firmware[9];
    uint64_t sectors;                       // 512-byte logical sectors (LBA48 count)
};

// slow_probe = the forensic build's port scan (see F4-06, forensic lab).
int init(PciAddr a, bool slow_probe);       // number of disks found, or a negative error
const DiskInfo& disk(int i);
// One command at a time on the first disk: 'count' sectors at 'lba' into/from 'buf'.
int rw(bool write, uint64_t lba, uint32_t count, void* buf);   // 0, or -E_IO after recovery
uint32_t interrupts();                      // interrupts taken (PCI INTx line)
void dump_port(int port);
}
