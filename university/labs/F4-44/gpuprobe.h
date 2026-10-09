// gpuprobe.h - DR405 F4-44 (milestone G3): a read-only inspection tool for display devices.
#pragma once
#include "../F4-02/pci.h"

namespace gpuprobe {
// Prints identity, BARs (decoded from their current values, never sized), capabilities and
// the expansion ROM. The only configuration writes are the ROM BAR's enable bit and its
// restore; they are counted and printed. No register of the device's BARs is written.
void inspect(PciAddr a);
void legacy_shadow();          // the VGA BIOS copy the PC firmware placed at 0xC0000
int config_writes();
}
