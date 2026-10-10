// f406_main.cc - DR301 F4-06: the AHCI driver's acceptance tests, and a boot timeline.
//   default:    IDENTIFY, the random-I/O test on AHCI and on virtio-blk in the same boot,
//               an error and its recovery
//   "boottrace": initialise every driver of the course in boot order and print one
//               timeline line per driver ("slowahci" selects the forensic port scan)
#include "../F4-01/kbase.h"
#include "../F4-03/i8042.h"
#include "../F4-03/irq.h"
#include "../F4-04/rtc.h"
#include "../F4-05/iotest.h"
#include "../F4-05/virtio_blk.h"
#include "ahci.h"

namespace {
PciAddr g_ahci{0xFF, 0, 0}, g_vblk{0xFF, 0, 0};
uint8_t g_gen[iotest::kBlocks];
alignas(4096) uint8_t g_buf[4096];
int g_last_rc = 0;

void find(PciAddr a)
{
    const uint32_t cc = pci::read32(a, pcireg::CLASS_REVISION) >> 8;
    if (cc == 0x010601 && g_ahci.bus == 0xFF) g_ahci = a;
    if (pci::read16(a, pcireg::VENDOR_ID) == 0x1AF4 && pci::read16(a, pcireg::DEVICE_ID) == 0x1042) g_vblk = a;
}

bool cmdline_has(uint32_t mbi, const char* word)
{
    const auto* info = reinterpret_cast<const uint32_t*>(mbi);
    if (!(info[0] & 0x4)) return false;
    const size_t n = kstrlen(word);
    for (const char* s = reinterpret_cast<const char*>(info[4]); *s; ++s)
        if (memcmp(s, word, n) == 0) return true;
    return false;
}

// AHCI through the iotest interface: one slot, the command completes inside start().
uint8_t* a_buf(int) { return g_buf; }
void a_start(int, bool write, uint32_t block) { g_last_rc = ahci::rw(write, uint64_t{block} * 8, 8, g_buf); }
int a_wait(bool& ok) { ok = g_last_rc == 0; return 0; }
uint8_t* v_buf(int s) { return vblk::slot_buffer(s); }
void v_start(int s, bool write, uint32_t block) { vblk::start(s, write, uint64_t{block} * 8); }
int v_wait(bool& ok) { uint8_t st; const int s = vblk::wait_any(st); ok = st == 0; return s; }

void report(const char* name, const iotest::Result& r)
{
    // 4 KiB per operation; KiB per emulated second, from the guest's own 1 ms tick
    const uint32_t kib = (r.reads + r.writes) * 4;
    kprintf("iotest %-10s qd=1: %u reads, %u writes, %u bad reads, %u errors, %u ms emulated, %u KiB/s\n",
            name, r.reads, r.writes, r.bad_reads, r.errors, static_cast<uint32_t>(r.ms),
            r.ms ? static_cast<uint32_t>(uint64_t{kib} * 1000 / r.ms) : 0);
}

void tests()
{
    if (g_ahci.bus == 0xFF || ahci::init(g_ahci, false) < 1) panic("no AHCI disk");
    const ahci::DiskInfo& d = ahci::disk(0);
    kprintf("IDENTIFY: model \"%s\" serial \"%s\" firmware \"%s\" sectors %lu (%lu MiB)\n", d.model, d.serial,
            d.firmware, d.sectors, d.sectors / 2048);
    bool pass = true;
    memset(g_gen, 0, sizeof g_gen);
    const iotest::Result ra = iotest::run(iotest::Disk{a_buf, a_start, a_wait, 1}, 4000, 1, 0x2F4A0006u, g_gen);
    report("ahci", ra);
    pass = pass && ra.bad_reads == 0 && ra.errors == 0;
    if (g_vblk.bus != 0xFF && vblk::init(g_vblk) == 0) {
        memset(g_gen, 0, sizeof g_gen);
        const iotest::Result rv = iotest::run(iotest::Disk{v_buf, v_start, v_wait, 1}, 4000, 1, 0x2F4A0006u, g_gen);
        report("virtio-blk", rv);
        pass = pass && rv.bad_reads == 0 && rv.errors == 0;
    }
    // An error on purpose: read the sector just past the end of the disk.
    kprintf("error test: reading LBA %lu (one past the last sector)\n", d.sectors);
    const int bad = ahci::rw(false, d.sectors, 1, g_buf);
    const int good = ahci::rw(false, 0, 1, g_buf);
    kprintf("error test: bad read returned %d, the next read of LBA 0 returned %d\n", bad, good);
    pass = pass && bad != 0 && good == 0;
    kprintf("ahci interrupts taken: %u\n", ahci::interrupts());
    kprintf("C5 tests: %s\n", pass ? "PASS" : "FAIL");
    if (!pass) qemu_exit(0x01);
}

void boottrace(bool slow)
{
    // Every driver of the course in a fixed boot order; one line per driver.
    struct Step { const char* name; const char* phase; };
    auto line = [](const char* name, const char* phase, uint64_t t0, uint64_t t1) {
        kprintf("BOOTTRACE %s %s %u %u\n", name, phase, static_cast<uint32_t>(t0), static_cast<uint32_t>(t1 - t0));
    };
    uint64_t t = timer::ms();
    pci::enumerate(find);
    line("pci", "bus", t, timer::ms());
    t = timer::ms();
    rtc::init(0x32);
    (void)rtc::now();
    line("rtc-cmos", "platform", t, timer::ms());
    t = timer::ms();
    (void)ps2::init();
    line("i8042", "platform", t, timer::ms());
    t = timer::ms();
    (void)vblk::init(g_vblk);
    line("virtio-blk", "storage", t, timer::ms());
    t = timer::ms();
    (void)ahci::init(g_ahci, slow);
    line("ahci", "storage", t, timer::ms());
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbi)
{
    serial_init();
    kprintf("F4-06 kernel: magic=0x%x\n", magic);
    irq::init();
    timer::init();
    irq::enable();
    if (cmdline_has(mbi, "boottrace")) {
        boottrace(cmdline_has(mbi, "slowahci"));
    } else {
        pci::enumerate(find);
        tests();
    }
    kprintf("F4-06 done\n");
    qemu_exit(0x10);
}
