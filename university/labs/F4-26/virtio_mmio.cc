// virtio_mmio.cc - F4-26: virtio-mmio version 2 transport and a polled virtio-blk read.
#include "virtio_mmio.h"
#include "kprint.h"

namespace {

// ---- transport registers (offsets from the slot base) ----
constexpr uint32_t kMagic = 0x000, kVersion = 0x004, kDeviceId = 0x008, kVendorId = 0x00c,
                   kDevFeatures = 0x010, kDevFeaturesSel = 0x014, kDrvFeatures = 0x020,
                   kDrvFeaturesSel = 0x024, kQueueSel = 0x030, kQueueNumMax = 0x034, kQueueNum = 0x038,
                   kQueueReady = 0x044, kQueueNotify = 0x050, kStatus = 0x070, kQueueDescLo = 0x080,
                   kQueueDescHi = 0x084, kQueueDriverLo = 0x090, kQueueDriverHi = 0x094,
                   kQueueDeviceLo = 0x0a0, kQueueDeviceHi = 0x0a4, kConfig = 0x100;
constexpr uint32_t kMagicValue = 0x74726976;   // "virt" read as a little-endian 32-bit value
constexpr uint32_t kStAck = 1, kStDriver = 2, kStDriverOk = 4, kStFeaturesOk = 8;
constexpr uint16_t kDescNext = 1, kDescWrite = 2;
constexpr uint32_t kQ = 8;                     // queue size this driver uses

struct Desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags, next;
};
struct Avail {
    uint16_t flags, idx, ring[kQ], used_event;
};
struct UsedElem {
    uint32_t id, len;
};
struct Used {
    uint16_t flags, idx;
    UsedElem ring[kQ];
    uint16_t avail_event;
};
struct BlkReq {
    uint32_t type;         // 0 = read ("IN"), 1 = write ("OUT")
    uint32_t reserved;
    uint64_t sector;
};

alignas(4096) Desc g_desc[kQ];
alignas(4096) Avail g_avail;
alignas(4096) Used g_used;
BlkReq g_req;
volatile uint8_t g_status;
uint64_t g_base = 0;
uint16_t g_last_used = 0;

uint32_t rd(uint32_t off) { return *reinterpret_cast<volatile uint32_t*>(g_base + off); }
void wr(uint32_t off, uint32_t v) { *reinterpret_cast<volatile uint32_t*>(g_base + off) = v; }

// The ordering this driver needs, AArch64 instructions (see the chapter's barrier section):
//   publish_barrier: descriptor and ring writes visible to the device before the index write
//   doorbell_barrier: all of that complete before the MMIO write that tells the device to look
//   consume_barrier: read the used index before reading what the device wrote
inline void publish_barrier() { asm volatile("dmb oshst" : : : "memory"); }
inline void doorbell_barrier() { asm volatile("dsb st" : : : "memory"); }
inline void consume_barrier() { asm volatile("dmb oshld" : : : "memory"); }

const char* device_name(uint32_t id)
{
    switch (id) {
    case 0: return "(empty slot)";
    case 1: return "network";
    case 2: return "block";
    case 3: return "console";
    case 4: return "entropy";
    default: return "(other)";
    }
}

} // namespace

namespace vmmio {

uint64_t scan(const fdt::Blob& dt, uint32_t want)
{
    uint64_t found = 0;
    int slots = 0, used = 0;
    dt.for_each_node([&](const fdt::Node& n) {
        uint64_t base = 0, size = 0;
        if (!dt.is_compatible(n, "virtio,mmio") || !dt.reg(n, 0, &base, &size)) {
            return true;
        }
        ++slots;
        g_base = base;
        uint32_t id = rd(kDeviceId);
        if (rd(kMagic) == kMagicValue && id != 0) {
            ++used;
            kprintf("virtio-mmio slot %s: version %u, device %u (%s), vendor 0x%x\n", n.name, rd(kVersion), id,
                    device_name(id), rd(kVendorId));
            if (id == want && found == 0) {
                found = base;
            }
        }
        return true;
    });
    kprintf("virtio-mmio: %d slots in the devicetree, %d with a device\n", slots, used);
    g_base = found;
    return found;
}

bool blk_init(uint64_t base)
{
    g_base = base;
    if (rd(kVersion) != 2) {
        kprintf("virtio-mmio at 0x%lx is version %u (legacy): unsupported, this driver needs version 2\n", base,
                rd(kVersion));
        return false;
    }
    wr(kStatus, 0);                                  // reset
    wr(kStatus, kStAck);
    wr(kStatus, kStAck | kStDriver);
    wr(kDevFeaturesSel, 1);
    uint32_t hi = rd(kDevFeatures);
    if ((hi & 1) == 0) {                             // feature bit 32: VIRTIO_F_VERSION_1
        kprintf("device does not offer VERSION_1\n");
        return false;
    }
    wr(kDrvFeaturesSel, 0);
    wr(kDrvFeatures, 0);                             // no optional block features
    wr(kDrvFeaturesSel, 1);
    wr(kDrvFeatures, 1);                             // VERSION_1
    wr(kStatus, kStAck | kStDriver | kStFeaturesOk);
    if ((rd(kStatus) & kStFeaturesOk) == 0) {
        kprintf("device refused the features\n");
        return false;
    }
    wr(kQueueSel, 0);
    uint32_t max = rd(kQueueNumMax);
    if (max < kQ) {
        kprintf("queue 0 too small (%u)\n", max);
        return false;
    }
    wr(kQueueNum, kQ);
    auto lo = [](const void* p) { return static_cast<uint32_t>(reinterpret_cast<uint64_t>(p)); };
    auto hi32 = [](const void* p) { return static_cast<uint32_t>(reinterpret_cast<uint64_t>(p) >> 32); };
    wr(kQueueDescLo, lo(g_desc));
    wr(kQueueDescHi, hi32(g_desc));
    wr(kQueueDriverLo, lo(&g_avail));
    wr(kQueueDriverHi, hi32(&g_avail));
    wr(kQueueDeviceLo, lo(&g_used));
    wr(kQueueDeviceHi, hi32(&g_used));
    wr(kQueueReady, 1);
    wr(kStatus, kStAck | kStDriver | kStFeaturesOk | kStDriverOk);
    kprintf("virtio-blk at 0x%lx: queue 0 has %u entries (max %u), capacity %lu sectors\n", base, kQ, max,
            blk_capacity());
    return true;
}

uint64_t blk_capacity()
{
    return uint64_t{rd(kConfig)} | uint64_t{rd(kConfig + 4)} << 32;
}

bool blk_read(uint64_t sector, uint8_t* buf)
{
    g_req = BlkReq{0, 0, sector};
    g_status = 0xff;
    g_desc[0] = Desc{reinterpret_cast<uint64_t>(&g_req), sizeof g_req, kDescNext, 1};
    g_desc[1] = Desc{reinterpret_cast<uint64_t>(buf), 512, static_cast<uint16_t>(kDescWrite | kDescNext), 2};
    g_desc[2] = Desc{reinterpret_cast<uint64_t>(&g_status), 1, kDescWrite, 0};
    uint16_t idx = g_avail.idx;
    g_avail.ring[idx % kQ] = 0;                      // head of the chain
    publish_barrier();                               // (1) descriptors and ring entry first ...
    *reinterpret_cast<volatile uint16_t*>(&g_avail.idx) = static_cast<uint16_t>(idx + 1);   // ... then the index
    doorbell_barrier();                              // (2) all of it before the doorbell
    wr(kQueueNotify, 0);
    while (*reinterpret_cast<volatile uint16_t*>(&g_used.idx) == g_last_used) {
    }
    consume_barrier();                               // (3) then read what the device wrote
    ++g_last_used;
    return g_status == 0;
}

} // namespace vmmio
