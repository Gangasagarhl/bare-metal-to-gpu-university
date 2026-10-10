// f443_main.cc - DR405 F4-43 (milestone G2): find the virtio-gpu, read the display
// information and the EDID, show two composited frames and a cursor. Between frames the
// kernel waits for a key, so the run script can take a QEMU screen dump of each frame.
#include "../F4-01/kbase.h"
#include "virtio_gpu.h"
#include "scene.h"

namespace {
constexpr int MAXW = 1024, MAXH = 768;
alignas(4096) uint32_t g_fb[MAXW * MAXH];              // the guest framebuffer (backing memory)
alignas(4096) uint32_t g_cursor_px[64 * 64];
PciAddr g_gpu{0xFF, 0, 0};

void find(PciAddr a)
{
    if (pci::read16(a, pcireg::VENDOR_ID) == 0x1AF4 && pci::read16(a, pcireg::DEVICE_ID) == vgp::PCI_DEVICE_ID)
        g_gpu = a;
}

void expect(const char* what, uint32_t got, uint32_t want)
{
    kprintf("%-30s -> %s\n", what, vgpu::resp_name(got));
    if (got != want) panic("virtio-gpu command failed");
}

void wait_key()                                       // polled i8042: a key press and release
{
    for (int n = 0; n < 2;) {
        if (inb(0x64) & 1) {
            (void)inb(0x60);
            ++n;
        }
    }
    while (inb(0x64) & 1) (void)inb(0x60);
}

void show(const char* tag, int w, vgp::Rect r)
{
    char what[40] = "TRANSFER_TO_HOST_2D ";
    memcpy(what + 20, tag, kstrlen(tag) + 1);
#ifdef F443_SKIP_TRANSFER_ON_UPDATE
    if (kstrcmp(tag, "damage") != 0)
#endif
        expect(what, vgpu::transfer_2d(1, r, static_cast<uint32_t>(w) * 4), vgp::RESP_OK_NODATA);
    expect("RESOURCE_FLUSH", vgpu::flush(1, r), vgp::RESP_OK_NODATA);
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-43 kernel: magic=0x%x\n", magic);
    pci::enumerate(find);
    if (g_gpu.bus == 0xFF) panic("no virtio-gpu");
    kprintf("found virtio-gpu at %02x:%02x.%x\n", g_gpu.bus, g_gpu.dev, g_gpu.fn);
    if (vgpu::init(g_gpu) != 0) panic("virtio-gpu init failed");
    kprintf("config: num_scanouts=%u events_read=0x%x\n", vgpu::num_scanouts(), vgpu::events());

    vgp::RespDisplayInfo di;
    expect("GET_DISPLAY_INFO", vgpu::display_info(di), vgp::RESP_OK_DISPLAY_INFO);
    for (int i = 0; i < vgp::MAX_SCANOUTS; ++i)
        if (di.pmodes[i].enabled)
            kprintf("scanout %d: enabled, %ux%u at (%u,%u)\n", i, di.pmodes[i].r.width, di.pmodes[i].r.height,
                    di.pmodes[i].r.x, di.pmodes[i].r.y);
    int w = static_cast<int>(di.pmodes[0].r.width), h = static_cast<int>(di.pmodes[0].r.height);
    if (w <= 0 || h <= 0 || w > MAXW || h > MAXH) panic("scanout 0 size not supported by this driver");

    vgp::RespEdid ed;
    const uint32_t et = vgpu::edid(0, ed);
    kprintf("%-30s -> %s, %u bytes\n", "GET_EDID scanout 0", vgpu::resp_name(et), et == vgp::RESP_OK_EDID ? ed.size : 0);
    // Print the base block and its extension blocks: byte 126 counts the extensions.
    const uint32_t shown = et == vgp::RESP_OK_EDID ? (1u + ed.edid[126]) * 128u : 0;
    if (et == vgp::RESP_OK_EDID)
        for (uint32_t i = 0; i < shown && i < ed.size && i < 1024; i += 16) {
            kprintf("edid %03x:", i);
            for (uint32_t k = i; k < i + 16 && k < ed.size; ++k) kprintf(" %02x", ed.edid[k]);
            kprintf("\n");
        }

    const uint32_t bytes = static_cast<uint32_t>(w * h * 4);
    expect("RESOURCE_CREATE_2D id 1", vgpu::create_2d(1, vgp::FORMAT_B8G8R8X8_UNORM, w, h), vgp::RESP_OK_NODATA);
    expect("RESOURCE_ATTACH_BACKING id 1", vgpu::attach_backing(1, g_fb, bytes), vgp::RESP_OK_NODATA);
    expect("SET_SCANOUT 0 -> id 1", vgpu::set_scanout(0, 1, w, h), vgp::RESP_OK_NODATA);

    scene::Fb fb{g_fb, w, h};
    const vgp::Rect full{0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    scene::frame(fb, 1);
    show("full", w, full);
    kprintf("frame 1 ready: %dx%d, fnv1a 0x%08x\n", w, h, fnv1a(g_fb, bytes));
    wait_key();

    scene::frame(fb, 2);                                // the guest image is now complete...
    const scene::Box d = scene::clip(scene::damage_1_to_2(), fb);
    const vgp::Rect dr{static_cast<uint32_t>(d.x0), static_cast<uint32_t>(d.y0),
                       static_cast<uint32_t>(d.x1 - d.x0), static_cast<uint32_t>(d.y1 - d.y0)};
    kprintf("damage: x=%u y=%u w=%u h=%u (%u of %u pixels)\n", dr.x, dr.y, dr.width, dr.height,
            dr.width * dr.height, static_cast<uint32_t>(w * h));
    show("damage", w, dr);                           // ...but only the damage is sent
    kprintf("frame 2 ready: %dx%d, fnv1a 0x%08x\n", w, h, fnv1a(g_fb, bytes));
    wait_key();

    scene::cursor(g_cursor_px);
    expect("RESOURCE_CREATE_2D id 2", vgpu::create_2d(2, vgp::FORMAT_B8G8R8A8_UNORM, 64, 64), vgp::RESP_OK_NODATA);
    expect("RESOURCE_ATTACH_BACKING id 2", vgpu::attach_backing(2, g_cursor_px, sizeof g_cursor_px),
           vgp::RESP_OK_NODATA);
    expect("TRANSFER_TO_HOST_2D id 2", vgpu::transfer_2d(2, vgp::Rect{0, 0, 64, 64}, 64 * 4), vgp::RESP_OK_NODATA);
    vgpu::update_cursor(0, 2, 100, 100, 0, 0);
    vgpu::move_cursor(0, 300, 250);
    kprintf("cursor: UPDATE_CURSOR at (100,100), MOVE_CURSOR to (300,250) consumed by the device\n");
    kprintf("config: events_read=0x%x\n", vgpu::events());
    kprintf("F4-43 done\n");
    qemu_exit(0x10);
}
