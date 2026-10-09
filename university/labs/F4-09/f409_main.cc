// f409_main.cc - DR302 F4-09: milestone C8 in QEMU. Enumerates every device on the xHCI
// root ports, reads a line from a USB keyboard, reads a file from a FAT32 USB stick,
// writes and re-reads one sector, then handles a hot-plugged and removed device.
#include "../F4-01/kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-08/intr.h"
#include "../F4-08/dma.h"
#include "../F4-08/vtd.h"
#include "usbh.h"
#include "hid_kbd.h"
#include "bot.h"

namespace {
alignas(4096) uint8_t g_sector[4096];

uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | p[1] << 8); }
uint32_t le32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t{p[3]} << 24; }

// READ(10) with a log line when the device refuses, including the sense data.
bool rd(uint32_t lba, uint16_t n, void* buf)
{
    const int st = bot::read(lba, n, buf);
    if (st == 0) return true;
    uint8_t key = 0, asc = 0, ascq = 0;
    const bool sense = st == 1 && bot::request_sense(key, asc, ascq);
    kprintf("scsi: READ(10) LBA %u, %u block(s) -> CSW status %d", lba, n, st);
    if (sense) kprintf(", sense key 0x%x ASC 0x%02x ASCQ 0x%02x", key, asc, ascq);
    kprintf("\n");
    return false;
}

bool storage_test(usbh::Device& d)
{
    if (!bot::start(d)) return false;
    uint8_t inq[36] = {};
    const uint8_t inquiry[6] = {bot::INQUIRY, 0, 0, 0, sizeof inq, 0};
    const int si = bot::command(inquiry, 6, inq, sizeof inq, true);
    char vendor[9] = {}, product[17] = {};
    memcpy(vendor, inq + 8, 8);
    memcpy(product, inq + 16, 16);
    kprintf("scsi: INQUIRY status %d, peripheral type %u, removable %u, vendor \"%s\" product \"%s\"\n", si,
            inq[0] & 0x1F, inq[1] >> 7, vendor, product);
    const uint8_t tur[6] = {bot::TEST_UNIT_READY};
    kprintf("scsi: TEST UNIT READY status %d\n", bot::command(tur, 6, nullptr, 0, false));
    uint32_t last = 0, bs = 0;
    if (!bot::read_capacity(last, bs)) return false;
    kprintf("scsi: READ CAPACITY(10): last LBA %u, block size %u (%u MiB)\n", last, bs, (last + 1) / 2048);

    // FAT32 boot sector (BIOS parameter block) at LBA 0.
    if (!rd(0, 1, g_sector)) return false;
    const uint16_t bps = le16(g_sector + 11), reserved = le16(g_sector + 14);
    const uint8_t spc = g_sector[13], nfats = g_sector[16];
    const uint32_t fatsz = le32(g_sector + 36), root = le32(g_sector + 44);
    char oem[9] = {}, fstype[9] = {}, label[12] = {};
    memcpy(oem, g_sector + 3, 8);
    memcpy(label, g_sector + 71, 11);
    memcpy(fstype, g_sector + 82, 8);
    kprintf("fat: OEM \"%s\", type \"%s\", label \"%s\", %u bytes/sector, %u sectors/cluster, %u reserved, "
            "%u FATs of %u sectors, root cluster %u, signature 0x%02x%02x\n", oem, fstype, label, bps, spc,
            reserved, nfats, fatsz, root, g_sector[511], g_sector[510]);
    if (bps != 512 || spc == 0 || spc > 8) return false;
    const uint32_t data_start = reserved + nfats * fatsz;
    const uint32_t root_lba = data_start + (root - 2) * spc;
    if (!rd(root_lba, spc, g_sector)) return false;
    kprintf("fat: root directory cluster %u at LBA %u\n", root, root_lba);
    for (uint32_t off = 0; off < spc * 512u; off += 32) {
        const uint8_t* e = g_sector + off;
        if (e[0] == 0) break;                         // end of directory
        if (e[0] == 0xE5 || e[11] == 0x0F) continue;  // deleted entry or long-name piece
        char name[12] = {};
        memcpy(name, e, 11);
        const uint32_t cl = uint32_t{le16(e + 20)} << 16 | le16(e + 26), size = le32(e + 28);
        kprintf("fat: entry \"%s\" attributes 0x%02x, first cluster %u, %u bytes\n", name, e[11], cl, size);
        if (memcmp(name, "HELLO   TXT", 11) == 0 && size < 512) {
            static uint8_t file[4096];
            if (!rd(data_start + (cl - 2) * spc, 1, file)) return false;
            file[size] = 0;
            kprintf("fat: HELLO.TXT says: %s", reinterpret_cast<char*>(file));
        }
    }
    // Write test on the last block, then read it back.
    for (int i = 0; i < 512; ++i) g_sector[i] = static_cast<uint8_t>(i * 13 + 7);
    memcpy(g_sector, "DR302 F4-09 wrote this block", 28);
    const int ws = bot::write(last, 1, g_sector);
    memset(g_sector, 0, 512);
    const int rs = bot::read(last, 1, g_sector);
    const uint32_t h = fnv1a(g_sector, 512);
    char start[29] = {};
    memcpy(start, g_sector, 28);
    kprintf("scsi: WRITE(10) LBA %u status %d, READ(10) back status %d, fnv1a 0x%08x, starts \"%s\"\n", last, ws,
            rs, h, start);
    return ws == 0 && rs == 0;
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-09 kernel: magic=0x%x\n", magic);
    intr::init();
    uint64_t ecam;
    uint8_t b0, b1;
    if (acpi::ecam(ecam, b0, b1)) pci::use_ecam(ecam, b0, b1);
    PciAddr x;
    if (!xhci::find(x)) panic("no xHCI controller");
    if (dma::use_iommu()) {
        dma::attach(x);
        vtd::enable();
    } else {
        dma::use_identity();
    }
    const uint8_t vec = intr::alloc_vector();
    if (!xhci::init(x, vec)) panic("xhci init failed");
    usbh::print_protocols();

    usbh::Device* kbd = nullptr;
    usbh::Device* stick = nullptr;
    for (uint8_t p = 1; p <= xhci::info().max_ports; ++p) {
        usbh::Device* d = usbh::attach(p);
        if (!d) continue;
        usbh::print_descriptors(*d);
        const usb::InterfaceDescriptor* f = usbh::interface(*d, 0);
        if (f && f->bInterfaceClass == usb::CLASS_HID && f->bInterfaceProtocol == 1) kbd = d;
        if (f && f->bInterfaceClass == usb::CLASS_MASS_STORAGE) stick = d;
    }
    bool ok = kbd && stick;
    if (kbd && hidkbd::start(*kbd)) {
        kprintf("hid: ready, type a line\n");
        char line[64];
        const int n = hidkbd::read_line(*kbd, line, sizeof line, 20000);
        kprintf("hid: line typed: \"%s\" (%d characters)\n", line, n);
        ok = ok && n > 0;
    }
    if (stick && !storage_test(*stick)) { kprintf("storage: FAILED\n"); ok = false; }

    kprintf("hotplug: waiting for a new device\n");
    usbh::Device* hp = nullptr;
    const uint64_t until = clock::ms() + 20000;
    while (!hp && clock::ms() < until) {
        const uint8_t p = usbh::wait_port_change(static_cast<uint32_t>(until - clock::ms()));
        if (!p) break;
        // Port events from the first scan are still queued: skip ports that already have a device.
        if (p == (kbd ? kbd->port : 0) || p == (stick ? stick->port : 0)) {
            kprintf("hotplug: event for port %u (already enumerated), ignored\n", p);
            continue;
        }
        hp = usbh::attach(p);
    }
    if (hp) {
        usbh::print_descriptors(*hp);
        kprintf("hotplug: remove it now\n");
        bool gone = false;
        const uint64_t until2 = clock::ms() + 20000;
        while (!gone && clock::ms() < until2) {
            const uint8_t q = usbh::wait_port_change(static_cast<uint32_t>(until2 - clock::ms()));
            if (!q) break;
            const uint32_t sc = xhci::portsc(q);
            kprintf("hotplug: port %u changed, connected=%u\n", q, sc & 1);
            gone = q == hp->port && !(sc & 1);     // our own port reset also reports a change
        }
        if (gone) usbh::detach(*hp);
        else ok = false;
    } else {
        ok = false;
    }
    kprintf("xhci: %u MSI-X interrupts on vector 0x%02x\n", xhci::interrupts(), vec);
    kprintf("F4-09 %s\n", ok ? "done" : "FAILED");
    qemu_exit(ok ? 0x10 : 0x01);
}
