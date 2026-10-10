// f407_main.cc - DR301 F4-07: the NVMe driver's acceptance tests and the project's root.
//   default:  Identify, PRP-list transfers, random I/O with 1, 4 and 8 I/O queue pairs,
//             then the root file system (a USTAR archive) from namespace 2
//   "nophase": the forensic build's completion loop that ignores the phase tag
#include "../F4-01/kbase.h"
#include "../F4-03/irq.h"
#include "../F4-05/iotest.h"
#include "nvme.h"
#include "ustar.h"

namespace {
PciAddr g_nvme{0xFF, 0, 0};
uint8_t g_gen[iotest::kBlocks];
alignas(4096) uint8_t g_big[64 * 1024];
uint32_t g_root_files = 0;

void find(PciAddr a)
{
    if ((pci::read32(a, pcireg::CLASS_REVISION) >> 8) == 0x010802 && g_nvme.bus == 0xFF) g_nvme = a;   // NVMe
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

uint8_t* n_buf(int s) { return nvme::slot_buffer(s); }
void n_start(int s, bool write, uint32_t block) { nvme::start(s, write, 1, uint64_t{block} * 8); }  // ns 1: 512 B LBAs
int n_wait(bool& ok)
{
    uint16_t st = 0;
    const int s = nvme::wait_any(st);
    if (s < 0) {
        kprintf("iotest stopped: no completion; %u stale completions seen\n", nvme::stale_completions());
        qemu_exit(0x01);
    }
    ok = st == 0;
    return s;
}

// The PRP cases: 1 page (PRP1 only), 2 pages (PRP2 = second page), 16 pages (PRP2 = list).
bool prp_test()
{
    bool pass = true;
    const uint32_t sizes[3] = {4096, 8192, 65536};
    for (uint32_t size : sizes) {
        for (uint32_t i = 0; i < size; ++i) g_big[i] = static_cast<uint8_t>(i * 7 + size / 4096);
        const uint32_t want = fnv1a(g_big, size);
        const int w = nvme::rw(1, true, 0, g_big, size);
        memset(g_big, 0, size);
        const int r = nvme::rw(1, false, 0, g_big, size);
        const uint32_t got = fnv1a(g_big, size);
        kprintf("prp test: %u bytes (%u pages, %s): write %d, read %d, fnv1a %08x %s\n", size, size / 4096,
                size == 4096 ? "PRP1 only" : size == 8192 ? "PRP2 = 2nd page" : "PRP2 = PRP list", w, r, got,
                got == want ? "match" : "MISMATCH");
        pass = pass && w == 0 && r == 0 && got == want;
    }
    memset(g_big, 0, sizeof g_big);                      // leave LBA 0..127 zero for the image check
    pass = pass && nvme::rw(1, true, 0, g_big, sizeof g_big) == 0 && nvme::flush(1) == 0;
    return pass;
}

int root_read(uint64_t off, void* buf, uint32_t bytes) { return nvme::rw(2, false, off / 4096, buf, bytes); }

void root_entry(const ustar::Entry& e)
{
    if (e.type == '5') { kprintf("root: %s directory\n", e.path); return; }
    uint32_t h = 0;
    if (ustar::hash(root_read, e, h) != 0) { kprintf("root: %s read error\n", e.path); return; }
    kprintf("root: %s %lu bytes fnv1a %08x\n", e.path, e.size, h);
    ++g_root_files;
    if (kstrcmp(e.path, "etc/hostname") == 0) {
        char name[32] = {};
        const int n = ustar::read_file(root_read, e, name, sizeof name - 1);
        if (n > 0 && name[n - 1] == '\n') name[n - 1] = 0;
        kprintf("root: hostname is \"%s\"\n", name);
    }
}

void tests()
{
    if (g_nvme.bus == 0xFF) panic("no NVMe controller");
    const int rc = nvme::init(g_nvme, 1, false);
    if (rc != 0) { kprintf("nvme init failed: %d\n", rc); panic("nvme"); }
    const nvme::CtrlInfo& c = nvme::ctrl();
    kprintf("identify controller: vid 0x%04x serial \"%s\" model \"%s\" firmware \"%s\" nn %u mdts %u\n", c.vid,
            c.serial, c.model, c.firmware, c.nn, c.mdts);
    kprintf("identify: active namespaces: %d (from the active namespace list; nn is the highest possible ID)\n",
            nvme::namespaces());
    for (int i = 0; i < nvme::namespaces(); ++i) {
        const nvme::NsInfo& n = nvme::ns(i);
        kprintf("identify namespace %u: nsze %lu blocks of %u bytes (%lu MiB)\n", n.nsid, n.nsze, n.lba_bytes,
                n.nsze * n.lba_bytes / (1024 * 1024));
    }
    bool pass = prp_test();
    const int counts[3] = {1, 4, 8};
    const uint32_t seeds[3] = {0x2F4A0701u, 0x2F4A0704u, 0x2F4A0708u};
    for (int i = 0; i < 3; ++i) {
        if (nvme::set_queues(counts[i]) != 0) panic("set_queues");
        const iotest::Result r =
            iotest::run(iotest::Disk{n_buf, n_start, n_wait, nvme::SLOTS}, 4000, 32, seeds[i], g_gen);
        kprintf("iotest %d I/O queue pair(s), qd=32: %u reads, %u writes, %u bad reads, %u errors, %u ms emulated\n",
                nvme::queues(), r.reads, r.writes, r.bad_reads, r.errors, static_cast<uint32_t>(r.ms));
        pass = pass && r.bad_reads == 0 && r.errors == 0;
    }
    pass = pass && nvme::flush(1) == 0;
    // the project: the root file system on namespace 2
    const int entries = ustar::walk(root_read, root_entry);
    kprintf("root: ustar archive on nvme namespace 2: %d entries, %u files\n", entries, g_root_files);
    pass = pass && entries > 0 && nvme::stale_completions() == 0;
    kprintf("stale completions: %u\n", nvme::stale_completions());
    kprintf("C6 tests: %s\n", pass ? "PASS" : "FAIL");
    if (!pass) qemu_exit(0x01);
}

void forensic()
{
    if (g_nvme.bus == 0xFF || nvme::init(g_nvme, 1, true) != 0) panic("nvme");
    const iotest::Result r = iotest::run(iotest::Disk{n_buf, n_start, n_wait, nvme::SLOTS}, 200, 1, 0x2F4A07F0u, g_gen);
    kprintf("iotest qd=1: %u reads, %u writes, %u bad reads, %u errors; %u stale completions\n", r.reads, r.writes,
            r.bad_reads, r.errors, nvme::stale_completions());
    if (r.bad_reads || r.errors || nvme::stale_completions()) qemu_exit(0x01);
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t mbi)
{
    serial_init();
    kprintf("F4-07 kernel: magic=0x%x\n", magic);
    irq::init();
    timer::init();
    irq::enable();
    pci::enumerate(find);
    if (cmdline_has(mbi, "nophase")) forensic();
    else tests();
    kprintf("F4-07 done\n");
    qemu_exit(0x10);
}
