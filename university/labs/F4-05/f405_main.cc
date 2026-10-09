// f405_main.cc - DR301 F4-05: find the virtio devices, run the block test at queue
// depths 1 and 32, then send an ARP request through virtio-net and print the reply.
#include "../F4-01/kbase.h"
#include "../F4-03/irq.h"
#include "../F4-05/iotest.h"
#include "virtio_blk.h"
#include "virtio_net.h"

namespace {
PciAddr g_blk{0xFF, 0, 0}, g_net{0xFF, 0, 0};
uint8_t g_gen[iotest::kBlocks];                     // 256 KiB: last generation per block

void find(PciAddr a)
{
    if (pci::read16(a, pcireg::VENDOR_ID) != 0x1AF4) return;
    const uint16_t id = pci::read16(a, pcireg::DEVICE_ID);
    kprintf("found virtio %02x:%02x.%x device id 0x%04x (%s)\n", a.bus, a.dev, a.fn, id,
            id == 0x1042 ? "block, modern" : id == 0x1041 ? "network, modern" : "other");
    if (id == 0x1042) g_blk = a;
    if (id == 0x1041) g_net = a;
}

uint8_t* blk_buffer(int slot) { return vblk::slot_buffer(slot); }
void blk_start(int slot, bool write, uint32_t block) { vblk::start(slot, write, uint64_t{block} * 8); }
int blk_wait(bool& ok)
{
    uint8_t st;
    const int s = vblk::wait_any(st);
    ok = st == 0;                                    // 0 = OK, 1 = I/O error, 2 = unsupported
    return s;
}

void arp_test()
{
    const uint8_t* m = vnet::mac();
    const uint8_t me[4] = {10, 0, 2, 15}, gw[4] = {10, 0, 2, 2};   // QEMU user networking
    uint8_t f[42] = {};
    memset(f, 0xFF, 6);                              // to: everyone (broadcast)
    memcpy(f + 6, m, 6);                             // from: our MAC
    f[12] = 0x08; f[13] = 0x06;                      // EtherType ARP
    f[14] = 0; f[15] = 1; f[16] = 0x08; f[17] = 0;   // hardware Ethernet, protocol IPv4
    f[18] = 6; f[19] = 4; f[20] = 0; f[21] = 1;      // address lengths; operation 1 = request
    memcpy(f + 22, m, 6); memcpy(f + 28, me, 4);     // sender MAC and IP
    memcpy(f + 38, gw, 4);                           // target IP (target MAC unknown: zero)
    vnet::send(f, sizeof f);
    kprintf("arp: sent who-has 10.0.2.2 tell 10.0.2.15\n");
    uint8_t r[1514];
    for (int tries = 0; tries < 8; ++tries) {
        const size_t n = vnet::receive(r, sizeof r, 50000000);
        if (n == 0) break;
        if (n >= 42 && r[12] == 0x08 && r[13] == 0x06 && r[21] == 2) {
            kprintf("arp: reply from %u.%u.%u.%u is-at %02x:%02x:%02x:%02x:%02x:%02x (%u bytes)\n", r[28],
                    r[29], r[30], r[31], r[22], r[23], r[24], r[25], r[26], r[27], static_cast<uint32_t>(n));
            return;
        }
        kprintf("net: ignored a %u-byte frame of type 0x%02x%02x\n", static_cast<uint32_t>(n), r[12], r[13]);
    }
    panic("no ARP reply");
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-05 kernel: magic=0x%x\n", magic);
    irq::init();
    timer::init();
    irq::enable();
    pci::enumerate(find);
    if (g_blk.bus == 0xFF || vblk::init(g_blk) != 0) panic("no virtio-blk");
    if (vblk::capacity_sectors() < uint64_t{iotest::kBlocks} * 8) panic("disk smaller than 1 GiB");

    const iotest::Disk disk{blk_buffer, blk_start, blk_wait, vblk::SLOTS};
    const int depths[2] = {1, 32};
    const uint32_t seeds[2] = {0x2F4A0001u, 0x2F4A0032u};
    bool pass = true;
    for (int i = 0; i < 2; ++i) {
        const iotest::Result r = iotest::run(disk, 4000, depths[i], seeds[i], g_gen);
        kprintf("iotest qd=%d: %u reads, %u writes over 1 GiB, %u bad reads, %u errors, %u ms emulated\n",
                depths[i], r.reads, r.writes, r.bad_reads, r.errors, static_cast<uint32_t>(r.ms));
        pass = pass && r.bad_reads == 0 && r.errors == 0;
    }
    kprintf("block test: %s\n", pass ? "PASS" : "FAIL");

    if (g_net.bus != 0xFF && vnet::init(g_net) == 0) arp_test();
    kprintf("F4-05 done\n");
    qemu_exit(pass ? 0x10 : 0x01);
}
