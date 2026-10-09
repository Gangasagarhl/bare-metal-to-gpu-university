// vtd.h - DR302 F4-08: a small Intel VT-d (DMA remapping) driver.
// Register offsets, table formats and fault-record layout written from the author's memory
// of the Intel Virtualization Technology for Directed I/O Architecture Specification
// ("Register Descriptions", "Root Entry", "Context Entry", "Second-Level Paging
// Entries", "Fault Recording Registers") and the ACPI DMAR table described there.
// Not opened in this build: confirmed only by QEMU's intel-iommu model (see F4-08).
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace vtdreg {
constexpr uint32_t VER = 0x00, CAP = 0x08, ECAP = 0x10, GCMD = 0x18, GSTS = 0x1C, RTADDR = 0x20,
                   CCMD = 0x28, FSTS = 0x34, FECTL = 0x38, FEDATA = 0x3C, FEADDR = 0x40,
                   FEUADDR = 0x44;
constexpr uint32_t GCMD_TE = 1u << 31, GCMD_SRTP = 1u << 30;       // GSTS: TES, RTPS (same bits)
constexpr uint64_t CCMD_ICC = 1ull << 63, CCMD_GLOBAL = 1ull << 61;
constexpr uint64_t IOTLB_IVT = 1ull << 63, IOTLB_GLOBAL = 1ull << 60;
constexpr uint32_t FSTS_PFO = 1u << 0, FSTS_PPF = 1u << 1;
constexpr uint32_t FECTL_IM = 1u << 31;
inline uint32_t cap_sagaw(uint64_t c) { return (c >> 8) & 0x1F; }   // bit 1: 39-bit, 3 levels
inline uint32_t cap_mgaw(uint64_t c) { return ((c >> 16) & 0x3F) + 1; }
inline uint32_t cap_fro(uint64_t c) { return ((c >> 24) & 0x3FF) * 16; }
inline uint32_t cap_nfr(uint64_t c) { return ((c >> 40) & 0xFF) + 1; }
inline uint32_t cap_nd(uint64_t c) { return c & 7; }               // domains: 2^(4 + 2*ND)
inline bool cap_cm(uint64_t c) { return (c >> 7) & 1; }            // caching mode
inline uint32_t ecap_iro(uint64_t e) { return ((e >> 8) & 0x3FF) * 16; }
inline bool ecap_pt(uint64_t e) { return (e >> 6) & 1; }
// Second-level page-table entry bits
constexpr uint64_t PTE_R = 1, PTE_W = 2, PTE_ADDR = 0x000FFFFFFFFFF000ull;
}

struct VtdFault {
    uint64_t addr;              // faulting page address (fault info, bits 63:12)
    uint16_t source_id;         // requester: bus << 8 | device << 3 | function
    uint8_t reason;             // fault reason code
    bool read;                  // type bit: 1 = read request, 0 = write request
    uint64_t raw_lo, raw_hi;    // the 128-bit record as read
};

namespace vtd {
bool init();                                    // DMAR table, register base, capability print
bool present();
void attach(PciAddr dev, uint16_t domain_id);   // context entry -> the device's own page table
void map_page(PciAddr dev, uint64_t iova, uint64_t phys, bool writable);
void unmap_page(PciAddr dev, uint64_t iova);
void flush_iotlb();                             // global IOTLB invalidation
void enable();                                  // root table pointer, invalidate, translation on
void fault_irq(uint8_t vector);                 // fault events as an MSI to this vector
int read_faults(VtdFault* out, int max);        // copies and clears pending fault records
const char* reason_text(uint8_t r);
void dump_walk(PciAddr dev, uint64_t iova);     // prints the three-level walk for one IOVA
}
