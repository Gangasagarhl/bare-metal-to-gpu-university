// virtio.cc - DR301 F4-05: the generic virtio-pci transport and split virtqueues.
#include "virtio.h"
#include "kbase.h"
#include "driver.h"

namespace {
uintptr_t bar_address(PciAddr a, uint8_t bar)
{
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    return static_cast<uintptr_t>(bars[bar].addr);   // paging off: physical = virtual
}
}  // namespace

namespace vio {
int init(Device& d, PciAddr a, uint64_t wanted)
{
    d.pci = a;
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER);   // DMA needs bus mastering
    // 1. Walk the vendor-specific capabilities: each points into a BAR.
    uint8_t p = pci::read8(a, pcireg::CAP_PTR) & 0xFC;
    for (int guard = 0; p && guard < 48; ++guard) {
        if (pci::read8(a, p) == pcireg::CAP_VENDOR) {
            const uint8_t type = pci::read8(a, static_cast<uint16_t>(p + 3));
            const uint8_t bar = pci::read8(a, static_cast<uint16_t>(p + 4));
            const uint32_t off = pci::read32(a, static_cast<uint16_t>(p + 8));
            const uintptr_t where = bar_address(a, bar) + off;
            if (type == CAP_COMMON && !d.common) d.common = reinterpret_cast<volatile CommonCfg*>(where);
            if (type == CAP_ISR && !d.isr) d.isr = reinterpret_cast<volatile uint8_t*>(where);
            if (type == CAP_DEVICE && !d.device_cfg) d.device_cfg = reinterpret_cast<volatile uint8_t*>(where);
            if (type == CAP_NOTIFY && !d.notify_base) {
                d.notify_base = where;
                d.notify_mult = pci::read32(a, static_cast<uint16_t>(p + 16));
            }
        }
        p = pci::read8(a, static_cast<uint16_t>(p + 1)) & 0xFC;
    }
    if (!d.common || !d.notify_base || !d.isr) return -E_NODEV;
    volatile CommonCfg& c = *d.common;
    // 2. Reset, then say "I see you" and "I have a driver".
    c.device_status = 0;
    while (c.device_status != 0) { }
    c.device_status = S_ACKNOWLEDGE;
    c.device_status = S_ACKNOWLEDGE | S_DRIVER;
    // 3. Feature negotiation: 64 bits read and written as two 32-bit halves.
    c.device_feature_select = 0;
    uint64_t offered = c.device_feature;
    c.device_feature_select = 1;
    offered |= uint64_t{c.device_feature} << 32;
    const uint64_t take = offered & (wanted | (uint64_t{1} << F_VERSION_1));
    if (!(take & (uint64_t{1} << F_VERSION_1))) { c.device_status = S_FAILED; return -E_NODEV; }
    c.driver_feature_select = 0;
    c.driver_feature = static_cast<uint32_t>(take);
    c.driver_feature_select = 1;
    c.driver_feature = static_cast<uint32_t>(take >> 32);
    c.device_status = S_ACKNOWLEDGE | S_DRIVER | S_FEATURES_OK;
    if (!(c.device_status & S_FEATURES_OK)) { c.device_status = S_FAILED; return -E_IO; }
    d.features = take;
    kprintf("virtio %02x:%02x.%x: device offers 0x%lx, driver accepted 0x%lx\n", a.bus, a.dev, a.fn,
            offered, take);
    return 0;
}

int setup_queue(Device& d, Queue& q, uint16_t index)
{
    volatile CommonCfg& c = *d.common;
    c.queue_select = index;
    const uint16_t device_max = c.queue_size;    // the device's maximum for this queue
    uint16_t size = device_max;
    if (size == 0) return -E_NODEV;
    if (size > QMAX) size = QMAX;
    c.queue_size = size;                         // the driver may choose a smaller size
    q.size = size;
    q.index = index;
    for (uint16_t i = 0; i < size; ++i) q.next_free[i] = static_cast<uint16_t>(i + 1);
    q.free_head = 0;
    q.num_free = size;
    q.last_used = 0;
    q.avail.flags = 1;                           // "no interrupt please": this driver polls
    q.avail.idx = 0;
    q.used.idx = 0;
    const auto da = reinterpret_cast<uintptr_t>(q.desc), aa = reinterpret_cast<uintptr_t>(&q.avail),
               ua = reinterpret_cast<uintptr_t>(&q.used);
    c.queue_desc_lo = da; c.queue_desc_hi = 0;
    c.queue_driver_lo = aa; c.queue_driver_hi = 0;
    c.queue_device_lo = ua; c.queue_device_hi = 0;
    q.notify = reinterpret_cast<volatile uint16_t*>(d.notify_base + c.queue_notify_off * d.notify_mult);
    c.queue_enable = 1;
    kprintf("virtio queue %u: size %u (device max %u), desc 0x%x avail 0x%x used 0x%x\n", index, size,
            device_max, static_cast<uint32_t>(da), static_cast<uint32_t>(aa), static_cast<uint32_t>(ua));
    return 0;
}

void driver_ok(Device& d) { d.common->device_status = S_ACKNOWLEDGE | S_DRIVER | S_FEATURES_OK | S_DRIVER_OK; }

int alloc_chain(Queue& q, int n)
{
    if (q.num_free < n) return -1;
    const uint16_t head = q.free_head;
    uint16_t i = head;
    for (int k = 0; k < n; ++k) {
        const uint16_t nx = q.next_free[i];
        q.desc[i].flags = (k + 1 < n) ? DESC_NEXT : 0;
        q.desc[i].next = (k + 1 < n) ? nx : 0;
        if (k + 1 < n) i = nx;
        else q.free_head = nx;
    }
    q.num_free = static_cast<uint16_t>(q.num_free - n);
    return head;
}

void free_chain(Queue& q, uint16_t head)
{
    uint16_t i = head;
    for (;;) {
        const bool more = q.desc[i].flags & DESC_NEXT;
        const uint16_t nx = q.desc[i].next;
        q.next_free[i] = q.free_head;
        q.free_head = i;
        ++q.num_free;
        if (!more) break;
        i = nx;
    }
}

void submit(Queue& q, uint16_t head)
{
    q.avail.ring[q.avail.idx % q.size] = head;
    // The device may read the ring as soon as idx changes: the entry must be written first.
    // On x86 stores stay in order; the compiler barrier stops g++ from reordering them.
    compiler_barrier();
    q.avail.idx = static_cast<uint16_t>(q.avail.idx + 1);
}

void kick(Queue& q)
{
    compiler_barrier();
    *q.notify = q.index;                         // write the queue index to its notify address
}

bool get_used(Queue& q, uint32_t& head, uint32_t& len)
{
    const uint16_t idx = *reinterpret_cast<volatile uint16_t*>(&q.used.idx);
    if (idx == q.last_used) return false;
    compiler_barrier();                          // read the element only after seeing idx
    const UsedElem& e = q.used.ring[q.last_used % q.size];
    head = e.id;
    len = e.len;
    q.last_used = static_cast<uint16_t>(q.last_used + 1);
    return true;
}
}  // namespace vio
