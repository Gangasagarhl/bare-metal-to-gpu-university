// bind4.cc - the DR401 lab kernel's driver match table (see bind4.h).
// The class codes are the curriculum's examples in section 5.2 (NVMe 01 08 02, AHCI 01 06 01,
// xHCI 0C 03 30, HD Audio 04 03) and vendor 1AF4 for virtio; the normative list is the PCI Code
// and ID Assignment Specification (title only in this build). The drivers here only CLAIM the
// function (probe == nullptr): this is a binding table, not a set of working drivers.
#include "bind4.h"

namespace bind4 {

static const Driver kTable[] = {
    {"e1000 (ID table)", Match::ById, 0x8086, 0x100E, 0, 0, 0, "Intel 8254x Software Developer's Manual", nullptr},
    {"virtio (vendor 1AF4)", Match::ByVendor, 0x1AF4, 0, 0, 0, 0, "OASIS VIRTIO Specification", nullptr},
    {"nvme (class)", Match::ByClass, 0, 0, 0x01, 0x08, 0x02, "NVM Express Base Specification", nullptr},
    {"ahci (class)", Match::ByClass, 0, 0, 0x01, 0x06, 0x01, "Serial ATA AHCI Specification", nullptr},
    {"xhci (class)", Match::ByClass, 0, 0, 0x0C, 0x03, 0x30, "xHCI Specification", nullptr},
    {"hda (class)", Match::ByClass, 0, 0, 0x04, 0x03, 0xFF, "High Definition Audio Specification", nullptr},
    {"host-bridge (class, inventory only)", Match::ByClass, 0, 0, 0x06, 0x00, 0xFF, "chipset datasheet", nullptr},
    {"isa-bridge (class, inventory only)", Match::ByClass, 0, 0, 0x06, 0x01, 0xFF, "chipset datasheet", nullptr},
};

const Driver* table(int& n)
{
    n = static_cast<int>(sizeof kTable / sizeof kTable[0]);
    return kTable;
}

const Driver* find(const pci4::Function& f)
{
    static const Match kPasses[] = {Match::ById, Match::ByVendor, Match::ByClass};
    for (const Match pass : kPasses) {
        for (const Driver& d : kTable) {
            if (d.how != pass) continue;
            const bool hit =
                (pass == Match::ById && d.vendor == f.vendor && d.device == f.device) ||
                (pass == Match::ByVendor && d.vendor == f.vendor) ||
                (pass == Match::ByClass && d.base == f.base_class && d.sub == f.sub_class &&
                 (d.prog_if == 0xFF || d.prog_if == f.prog_if));
            if (hit) return &d;
        }
    }
    return nullptr;
}

} // namespace bind4
