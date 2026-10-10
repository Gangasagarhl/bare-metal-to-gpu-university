// virtio9p.cc - DS403 kernel, F5-45: legacy ("transitional") virtio over PCI port I/O.
// Register offsets and the split-ring layout follow the VIRTIO specification's legacy
// interface section as recalled (see the unverified box in F5-45); this file was then
// checked only by working against QEMU 8.2.2's virtio-9p-pci device.
#include "virtio9p.h"
#include "k.h"

namespace {
inline void outw(uint16_t p, uint16_t v) { asm volatile("outw %0, %1" : : "a"(v), "Nd"(p)); }
inline uint16_t inw(uint16_t p)
{
    uint16_t v;
    asm volatile("inw %1, %0" : "=a"(v) : "Nd"(p));
    return v;
}

constexpr uint16_t HOST_FEATURES = 0x00, GUEST_FEATURES = 0x04, QUEUE_PFN = 0x08,
                   QUEUE_SIZE = 0x0C, QUEUE_SEL = 0x0E, QUEUE_NOTIFY = 0x10, STATUS = 0x12,
                   CONFIG = 0x14;                         // device config when MSI-X is off
constexpr uint8_t S_ACK = 1, S_DRIVER = 2, S_DRIVER_OK = 4;

struct Desc { uint64_t addr; uint32_t len; uint16_t flags; uint16_t next; };
constexpr uint16_t F_NEXT = 1, F_WRITE = 2;

alignas(4096) uint8_t ring[4 * 4096];                    // room for a queue of up to 128
uint16_t io = 0, qsz = 0, last_used = 0;
Desc* desc = nullptr;
volatile uint16_t* avail = nullptr;                      // flags, idx, ring[qsz]
volatile uint8_t* used = nullptr;                        // flags, idx, ring[qsz] of {id, len}

uint32_t pci_read(uint8_t dev, uint8_t off)
{
    outl(0xCF8, 0x80000000u | (dev << 11) | (off & 0xFC));
    return inl(0xCFC);
}
}  // namespace

bool v9p_init(char* tag, int cap)
{
    int found = -1;
    for (int d = 0; d < 32 && found < 0; ++d)
        if (pci_read(static_cast<uint8_t>(d), 0) == 0x10091AF4u) found = d;
    if (found < 0) return false;
    uint8_t d = static_cast<uint8_t>(found);
    io = static_cast<uint16_t>(pci_read(d, 0x10) & ~0x3u);
    outl(0xCF8, 0x80000000u | (d << 11) | 0x04);
    outl(0xCFC, inl(0xCFC) | 0x5);                       // I/O space + bus master

    outb(io + STATUS, 0);                                // reset
    outb(io + STATUS, S_ACK);
    outb(io + STATUS, S_ACK | S_DRIVER);
    uint32_t features = inl(io + HOST_FEATURES);
    outl(io + GUEST_FEATURES, features & 1u);            // bit 0: the device has a mount tag
    outw(io + QUEUE_SEL, 0);
    qsz = inw(io + QUEUE_SIZE);
    if (qsz == 0 || qsz > 128) return false;
    desc = reinterpret_cast<Desc*>(ring);
    avail = reinterpret_cast<volatile uint16_t*>(ring + 16u * qsz);
    uint32_t avail_end = 16u * qsz + 2u * (3u + qsz);
    used = ring + ((avail_end + 4095u) & ~4095u);       // the used ring starts on a new page
    outl(io + QUEUE_PFN, reinterpret_cast<uint32_t>(ring) >> 12);
    outb(io + STATUS, S_ACK | S_DRIVER | S_DRIVER_OK);

    int n = 0;
    if (features & 1u) {
        uint16_t len = static_cast<uint16_t>(inb(io + CONFIG) | (inb(io + CONFIG + 1) << 8));
        for (; n < len && n + 1 < cap; ++n) tag[n] = static_cast<char>(inb(static_cast<uint16_t>(io + CONFIG + 2 + n)));
    }
    tag[n] = '\0';
    klog("virtio-9p: I/O base 0x%x, queue size %u, features 0x%x", io, qsz, features);
    return true;
}

int v9p_call(const uint8_t* req, uint32_t len, uint8_t* rep, uint32_t cap)
{
    desc[0] = Desc{reinterpret_cast<uint32_t>(req), len, F_NEXT, 1};
    desc[1] = Desc{reinterpret_cast<uint32_t>(rep), cap, F_WRITE, 0};
    uint16_t idx = avail[1];
    avail[2 + idx % qsz] = 0;                            // head of the chain: descriptor 0
    asm volatile("" : : : "memory");
    avail[1] = static_cast<uint16_t>(idx + 1);
    outw(io + QUEUE_NOTIFY, 0);
    for (uint32_t spin = 0; spin < 200000000u; ++spin) {
        uint16_t uidx = *reinterpret_cast<volatile uint16_t*>(used + 2);
        if (uidx != last_used) {
            uint32_t slot = last_used % qsz;
            uint32_t got = *reinterpret_cast<volatile uint32_t*>(used + 4 + 8 * slot + 4);
            last_used = uidx;
            return static_cast<int>(got);
        }
    }
    return -1;
}
