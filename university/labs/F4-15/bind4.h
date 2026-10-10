// bind4.h - a driver match table for the DR401 lab kernel: which driver claims which PCI
// function, by exact ID, by vendor, or by class code (the order a real driver model uses:
// the most specific match wins).
#pragma once
#include "pci4.h"

namespace bind4 {

enum class Match { ById, ByVendor, ByClass };

struct Driver {
    const char* name;
    Match how;
    uint16_t vendor, device;          // ById / ByVendor
    uint8_t base, sub, prog_if;       // ByClass; 0xFF in prog_if = any
    const char* standard;             // the document a class match points to
    bool (*probe)(const pci4::Function& f);   // nullptr: claim only, no initialisation
};

// The table: exact IDs first, then vendor matches, then class matches.
const Driver* table(int& n);
const Driver* find(const pci4::Function& f);

} // namespace bind4
