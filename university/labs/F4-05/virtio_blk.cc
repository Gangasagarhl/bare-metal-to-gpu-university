// virtio_blk.cc - DR301 F4-05: virtio block device. Each request is a chain of three
// descriptors: a header the device reads, the data buffer, and a status byte it writes.
#include "virtio_blk.h"
#include "kbase.h"
#include "driver.h"
#include "virtio.h"

namespace {
struct ReqHeader { uint32_t type, reserved; uint64_t sector; };   // type 0 = read, 1 = write
static_assert(sizeof(ReqHeader) == 16, "virtio-blk request header is 16 bytes");
constexpr uint32_t T_IN = 0, T_OUT = 1;

vio::Device g_dev;
vio::Queue g_q;
alignas(4096) uint8_t g_data[vblk::SLOTS][4096];
ReqHeader g_hdr[vblk::SLOTS];
volatile uint8_t g_status[vblk::SLOTS];
int g_head_to_slot[vio::QMAX];
uint64_t g_capacity = 0;
}  // namespace

namespace vblk {
int init(PciAddr a)
{
    int rc = vio::init(g_dev, a, 0);           // no optional features: plain reads and writes
    if (rc) return rc;
    if ((rc = vio::setup_queue(g_dev, g_q, 0))) return rc;
    // device configuration: the capacity in 512-byte sectors is the first 64-bit field
    g_capacity = *reinterpret_cast<volatile uint32_t*>(g_dev.device_cfg) |
                 (uint64_t{*reinterpret_cast<volatile uint32_t*>(g_dev.device_cfg + 4)} << 32);
    vio::driver_ok(g_dev);
    kprintf("virtio-blk: capacity %lu sectors (%lu MiB)\n", g_capacity, g_capacity / 2048);
    return 0;
}

uint64_t capacity_sectors() { return g_capacity; }
uint8_t* slot_buffer(int slot) { return g_data[slot]; }

void start(int slot, bool write, uint64_t sector)
{
    g_hdr[slot] = ReqHeader{write ? T_OUT : T_IN, 0, sector};
    g_status[slot] = 0xFF;                     // the device overwrites it when done
    const int head = vio::alloc_chain(g_q, 3);
    if (head < 0) panic("virtio-blk: no free descriptors");
    vio::Desc* d = g_q.desc;
    const uint16_t h = static_cast<uint16_t>(head), dd = d[h].next, sd = d[dd].next;
    d[h].addr = reinterpret_cast<uintptr_t>(&g_hdr[slot]);
    d[h].len = sizeof(ReqHeader);              // flags: NEXT (set by alloc_chain)
    d[dd].addr = reinterpret_cast<uintptr_t>(g_data[slot]);
    d[dd].len = 4096;
#ifndef F405_READ_WITHOUT_WRITE_FLAG
    if (!write) d[dd].flags |= vio::DESC_WRITE; // the device writes into our buffer on a read
#endif
    d[sd].addr = reinterpret_cast<uintptr_t>(&g_status[slot]);
    d[sd].len = 1;
    d[sd].flags |= vio::DESC_WRITE;            // the status byte is always device-written
    g_head_to_slot[h] = slot;
    vio::submit(g_q, h);
    vio::kick(g_q);
}

int wait_any(uint8_t& status)
{
    uint32_t head, len;
    while (!vio::get_used(g_q, head, len)) { }  // polling: interrupts are F4-08's topic
    const int slot = g_head_to_slot[head];
    status = g_status[slot];
    vio::free_chain(g_q, static_cast<uint16_t>(head));
    return slot;
}
}  // namespace vblk
