// usbh.cc - DR302 F4-09: USB device management for the xHCI driver.
#include "usbh.h"
#include "../F4-01/kbase.h"
#include "../F4-08/intr.h"

namespace {
using namespace xhci;
constexpr uint32_t PORTSC_CCS = 1u << 0, PORTSC_PED = 1u << 1, PORTSC_PR = 1u << 4,
                   PORTSC_PRC = 1u << 21, PORTSC_CSC = 1u << 17;
// Bits that must never be written back as read: PED and the change bits are write-1-to-clear,
// PR and WPR start a reset, LWS enables link-state writes.
constexpr uint32_t PORTSC_NEUTRAL_CLEAR = 0x80FF0012u;
constexpr uint32_t TRB_IOC = 1u << 5, TRB_ISP = 1u << 2, TRB_IDT = 1u << 6, TRB_DIR_IN = 1u << 16;

usbh::Device g_dev[usbh::MAX_DEVICES];
alignas(4096) uint8_t g_in_ctx[usbh::MAX_DEVICES][4096];
alignas(4096) uint8_t g_out_ctx[usbh::MAX_DEVICES][4096];
alignas(4096) uint8_t g_xfer[usbh::MAX_DEVICES][4096];
alignas(4096) Trb g_rings[usbh::MAX_DEVICES][1 + usbh::MAX_EPS][64];
uint64_t g_xfer_iova[usbh::MAX_DEVICES];
Trb g_stash[8];                       // transfer/port events that arrived while waiting for others
int g_nstash = 0;

uint32_t neutral(uint32_t v) { return v & ~PORTSC_NEUTRAL_CLEAR; }

uint32_t* ctx(uint8_t* base, int index)            // index 0 = input control or slot context
{
    return reinterpret_cast<uint32_t*>(base + index * xhci::info().context_size);
}

int dev_index(const usbh::Device& d) { return static_cast<int>(&d - g_dev); }

// Waits for the transfer event of the TRB at 'iova'; returns it, or a zero TRB on timeout.
bool wait_transfer(uint8_t slot, uint64_t iova, Trb& out, uint32_t timeout_ms)
{
    for (int i = 0; i < g_nstash; ++i) {
        if (trb_type(g_stash[i]) == TRANSFER_EVENT && g_stash[i].param == iova) {
            out = g_stash[i];
            for (int j = i + 1; j < g_nstash; ++j) g_stash[j - 1] = g_stash[j];
            --g_nstash;
            return true;
        }
    }
    const uint64_t end = clock::ms() + timeout_ms;
    for (;;) {
        Trb ev;
        const uint64_t now = clock::ms();
        if (now > end || !wait_event(ev, static_cast<uint32_t>(end - now))) return false;
        if (trb_type(ev) == TRANSFER_EVENT && event_slot(ev) == slot && ev.param == iova) { out = ev; return true; }
        // A transfer event for an earlier TRB of the same request (short packet) or another event:
        if (trb_type(ev) == TRANSFER_EVENT && event_slot(ev) == slot && completion_code(ev) == CC_SHORT_PACKET) {
            if (g_nstash < 8) g_stash[g_nstash++] = ev;
            continue;
        }
        if (g_nstash < 8) g_stash[g_nstash++] = ev;
    }
}

void fill_ep_ctx(uint32_t* ep, uint8_t ep_type, uint16_t mps, uint8_t interval, uint64_t ring_iova)
{
    ep[0] = uint32_t{interval} << 16;
    ep[1] = (3u << 1) | (uint32_t{ep_type} << 3) | (uint32_t{mps} << 16);   // CErr = 3
    ep[2] = static_cast<uint32_t>(ring_iova) | 1;                           // DCS = 1
    ep[3] = static_cast<uint32_t>(ring_iova >> 32);
    ep[4] = (ep_type == 4 ? 8u : mps) | (ep_type == 7 || ep_type == 3 ? uint32_t{mps} << 16 : 0);
}
}  // namespace

namespace usbh {
const char* speed_name(uint8_t s)
{
    switch (s) {
    case 1: return "full";
    case 2: return "low";
    case 3: return "high";
    case 4: return "super";
    default: return "unknown";
    }
}

void print_protocols()
{
    const Info& in = xhci::info();
    uint32_t off = (mmio_read<uint32_t>(in.mmio + 0x10) >> 16) * 4;   // HCCPARAMS1.xECP (dwords)
    while (off) {
        const uint32_t c = mmio_read<uint32_t>(in.mmio + off);
        if ((c & 0xFF) == 2) {                                         // Supported Protocol
            const uint32_t name = mmio_read<uint32_t>(in.mmio + off + 4);
            const uint32_t ports = mmio_read<uint32_t>(in.mmio + off + 8);
            kprintf("xhci: protocol \"%c%c%c%c\" %x.%02x on ports %u-%u\n", name & 0xFF, (name >> 8) & 0xFF,
                    (name >> 16) & 0xFF, name >> 24, c >> 24, (c >> 16) & 0xFF, ports & 0xFF,
                    (ports & 0xFF) + ((ports >> 8) & 0xFF) - 1);
        }
        const uint32_t next = (c >> 8) & 0xFF;
        off = next ? off + next * 4 : 0;
    }
}

Device* attach(uint8_t port)
{
    uint32_t sc = portsc(port);
    if (!(sc & PORTSC_CCS)) return nullptr;
    set_portsc(port, neutral(sc) | PORTSC_CSC);            // acknowledge the connect change
    set_portsc(port, neutral(portsc(port)) | PORTSC_PR);   // USB 2 ports need a reset to enable
    const uint64_t end = clock::ms() + 500;
    while (!(portsc(port) & PORTSC_PRC))
        if (clock::ms() > end) { kprintf("usb: port %u reset timed out\n", port); return nullptr; }
    set_portsc(port, neutral(portsc(port)) | PORTSC_PRC);
    sc = portsc(port);
    if (!(sc & PORTSC_PED)) { kprintf("usb: port %u not enabled after reset\n", port); return nullptr; }

    const Trb es = command(Trb{0, 0, ENABLE_SLOT << 10});
    if (completion_code(es) != CC_SUCCESS) { kprintf("usb: Enable Slot failed, code %u\n", completion_code(es)); return nullptr; }
    const uint8_t slot = static_cast<uint8_t>(event_slot(es));
    int i = 0;
    while (i < MAX_DEVICES && g_dev[i].used) ++i;
    if (i == MAX_DEVICES) return nullptr;
    Device& d = g_dev[i];
    d = Device{};
    d.used = true;
    d.slot = slot;
    d.port = port;
    d.speed = (sc >> 10) & 0xF;
    d.in_ctx = g_in_ctx[i];
    d.out_ctx = g_out_ctx[i];
    d.xfer = g_xfer[i];
    if (!g_xfer_iova[i]) g_xfer_iova[i] = iova_of(g_xfer[i], 4096, true);
    memset(d.in_ctx, 0, 4096);
    memset(d.out_ctx, 0, 4096);
    ring_init(d.ep0, g_rings[i][0], 64);

    const uint16_t mps0 = d.speed == 4 ? 512 : d.speed == 3 ? 64 : 8;
    ctx(d.in_ctx, 0)[1] = 0x3;                                   // add slot context and EP0
    uint32_t* sl = ctx(d.in_ctx, 1);
    sl[0] = (1u << 27) | (uint32_t{d.speed} << 20);              // 1 context entry, speed, route 0
    sl[1] = uint32_t{port} << 16;                                // root hub port number
    fill_ep_ctx(ctx(d.in_ctx, 2), 4, mps0, 0, d.ep0.iova);       // EP type 4 = control
    static_cast<uint64_t*>(dcbaa())[slot] = iova_of(d.out_ctx, 4096, true);
    const Trb ad = command(Trb{iova_of(d.in_ctx, 4096, false), 0,
                               (ADDRESS_DEVICE << 10) | (uint32_t{slot} << 24)});
    if (completion_code(ad) != CC_SUCCESS) {
        kprintf("usb: Address Device failed on port %u, code %u\n", port, completion_code(ad));
        return nullptr;
    }
    const uint32_t addr = ctx(d.out_ctx, 0)[3] & 0xFF;
    kprintf("usb: port %u: %s-speed device, slot %u, USB address %u (slot state %u)\n", port,
            speed_name(d.speed), slot, addr, ctx(d.out_ctx, 0)[3] >> 27);

    // First 8 bytes of the device descriptor give bMaxPacketSize0, then the whole thing.
    if (control(d, usb::DIR_IN, usb::REQ_GET_DESCRIPTOR, usb::DT_DEVICE << 8, 0, &d.dd, 8) != 8) return nullptr;
    if (control(d, usb::DIR_IN, usb::REQ_GET_DESCRIPTOR, usb::DT_DEVICE << 8, 0, &d.dd, 18) != 18) return nullptr;
    usb::ConfigDescriptor cd;
    if (control(d, usb::DIR_IN, usb::REQ_GET_DESCRIPTOR, usb::DT_CONFIG << 8, 0, &cd, 9) != 9) return nullptr;
    d.config_len = cd.wTotalLength > sizeof d.config ? sizeof d.config : cd.wTotalLength;
    if (control(d, usb::DIR_IN, usb::REQ_GET_DESCRIPTOR, usb::DT_CONFIG << 8, 0, d.config, d.config_len) !=
        d.config_len) return nullptr;
    return &d;
}

void detach(Device& d)
{
    const Trb ev = command(Trb{0, 0, (DISABLE_SLOT << 10) | (uint32_t{d.slot} << 24)});
    kprintf("usb: slot %u disabled (code %u)\n", d.slot, completion_code(ev));
    static_cast<uint64_t*>(dcbaa())[d.slot] = 0;
    d.used = false;
}

int control(Device& d, uint8_t reqtype, uint8_t req, uint16_t value, uint16_t index, void* data,
            uint16_t length)
{
    const bool in = reqtype & usb::DIR_IN;
    const usb::Setup s{reqtype, req, value, index, length};
    uint64_t setup_bits;
    memcpy(&setup_bits, &s, 8);
    const uint32_t trt = length == 0 ? 0 : in ? 3 : 2;
    ring_push(d.ep0, Trb{setup_bits, 8, (SETUP << 10) | TRB_IDT | (trt << 16)});
    const int i = dev_index(d);
    uint64_t data_trb = 0;
    if (length) {
        if (!in) memcpy(d.xfer, data, length);
        data_trb = ring_push(d.ep0, Trb{g_xfer_iova[i], length,
                                        (DATA << 10) | TRB_ISP | (in ? TRB_DIR_IN : 0)});
    }
    const uint64_t status_trb = ring_push(d.ep0, Trb{0, 0, (STATUS << 10) | TRB_IOC |
                                                     ((length == 0 || !in) ? TRB_DIR_IN : 0)});
    doorbell(d.slot, 1);                                          // target 1 = EP0
    Trb ev;
    if (!wait_transfer(d.slot, status_trb, ev, 1000)) return -1000;
    if (completion_code(ev) != CC_SUCCESS) return -static_cast<int>(completion_code(ev));
    int got = length;
    Trb dev_ev;
    if (length && wait_transfer(d.slot, data_trb, dev_ev, 0)) got = length - (dev_ev.status & 0xFFFFFF);
    if (in && got > 0) memcpy(data, d.xfer, got);
    return got;
}

const usb::InterfaceDescriptor* interface(const Device& d, int n)
{
    for (int off = 0; off + 2 <= d.config_len && d.config[off] >= 2; off += d.config[off]) {
        if (d.config[off + 1] == usb::DT_INTERFACE && n-- == 0)
            return reinterpret_cast<const usb::InterfaceDescriptor*>(d.config + off);
    }
    return nullptr;
}

int configure(Device& d, uint8_t iface)
{
    const int i = dev_index(d);
    memset(d.in_ctx, 0, 4096);
    uint32_t add = 1;                                             // slot context
    uint8_t max_dci = 1;
    bool in_iface = false;
    for (int off = 0; off + 2 <= d.config_len && d.config[off] >= 2; off += d.config[off]) {
        const uint8_t type = d.config[off + 1];
        if (type == usb::DT_INTERFACE) in_iface = d.config[off + 2] == iface;
        if (type != usb::DT_ENDPOINT || !in_iface || d.neps == MAX_EPS) continue;
        const auto* e = reinterpret_cast<const usb::EndpointDescriptor*>(d.config + off);
        Endpoint& ep = d.eps[d.neps];
        ep.address = e->bEndpointAddress;
        ep.type = e->bmAttributes & 3;
        ep.mps = e->wMaxPacketSize & 0x7FF;
        ep.interval = e->bInterval;
        const bool is_in = ep.address & 0x80;
        ep.dci = static_cast<uint8_t>(2 * (ep.address & 0xF) + (is_in ? 1 : 0));
        ring_init(ep.ring, g_rings[i][1 + d.neps], 64);
        // xHCI endpoint type: 2 bulk OUT, 3 interrupt OUT, 6 bulk IN, 7 interrupt IN.
        const uint8_t xt = static_cast<uint8_t>((ep.type == usb::XFER_BULK ? 2 : 3) + (is_in ? 4 : 0));
        // Interval: high/super speed endpoints give an exponent (bInterval - 1); full/low
        // speed interrupt endpoints give frames, converted to 125 us units as log2(frames * 8).
        uint8_t ival = 0;
        if (ep.type == usb::XFER_INT) {
            if (d.speed >= 3) ival = static_cast<uint8_t>(ep.interval ? ep.interval - 1 : 0);
            else { uint32_t f = uint32_t{ep.interval} * 8; while (f > 1) { f >>= 1; ++ival; } }
        }
        fill_ep_ctx(ctx(d.in_ctx, 1 + ep.dci), xt, ep.mps, ival, ep.ring.iova);
        add |= 1u << ep.dci;
        if (ep.dci > max_dci) max_dci = ep.dci;
        kprintf("usb: slot %u endpoint 0x%02x %s %s, max packet %u, interval field %u -> DCI %u\n", d.slot,
                ep.address, ep.type == usb::XFER_BULK ? "bulk" : "interrupt", is_in ? "IN" : "OUT", ep.mps, ival,
                ep.dci);
        ++d.neps;
    }
    memcpy(ctx(d.in_ctx, 1), ctx(d.out_ctx, 0), 32);              // current slot context ...
    uint32_t* sl = ctx(d.in_ctx, 1);
    sl[0] = (sl[0] & ~(0x1Fu << 27)) | (uint32_t{max_dci} << 27); // ... with more entries
    sl[3] = 0;                                                     // address/state: controller-owned
    ctx(d.in_ctx, 0)[1] = add;
    const Trb ce = command(Trb{iova_of(d.in_ctx, 4096, false), 0,
                               (CONFIGURE_ENDPOINT << 10) | (uint32_t{d.slot} << 24)});
    if (completion_code(ce) != CC_SUCCESS) {
        kprintf("usb: Configure Endpoint failed, code %u\n", completion_code(ce));
        return -1;
    }
    const auto* cd = reinterpret_cast<const usb::ConfigDescriptor*>(d.config);
    if (control(d, usb::DIR_OUT, usb::REQ_SET_CONFIGURATION, cd->bConfigurationValue, 0, nullptr, 0) < 0)
        return -1;
    return d.neps;
}

Endpoint* find_ep(Device& d, uint8_t type, bool in)
{
    for (int i = 0; i < d.neps; ++i)
        if (d.eps[i].type == type && ((d.eps[i].address & 0x80) != 0) == in) return &d.eps[i];
    return nullptr;
}

int transfer(Device& d, Endpoint& ep, void* data, uint32_t length, uint32_t timeout_ms)
{
    if (length > 4096) return -1001;
    const bool in = ep.address & 0x80;
    const int i = dev_index(d);
    if (!in) memcpy(d.xfer, data, length);
    const uint64_t at = ring_push(ep.ring, Trb{g_xfer_iova[i], length, (NORMAL << 10) | TRB_IOC | TRB_ISP});
    doorbell(d.slot, ep.dci);
    Trb ev;
    if (!wait_transfer(d.slot, at, ev, timeout_ms)) return -1000;
    const uint32_t cc = completion_code(ev);
    if (cc != CC_SUCCESS && cc != CC_SHORT_PACKET) return -static_cast<int>(cc);
    const int got = static_cast<int>(length - (ev.status & 0xFFFFFF));
    if (in && got > 0) memcpy(data, d.xfer, got);
    return got;
}

void print_descriptors(const Device& d)
{
    const usb::DeviceDescriptor& dd = d.dd;
    // USB 3 devices give bMaxPacketSize0 as a power of two (9 -> 512 bytes).
    const uint32_t mps0 = dd.bcdUSB >= 0x300 ? 1u << dd.bMaxPacketSize0 : dd.bMaxPacketSize0;
    kprintf("  device: USB %x.%02x class %u/%u/%u, EP0 max packet %u, vendor 0x%04x product 0x%04x, "
            "bcdDevice %x.%02x, %u configuration(s)\n", dd.bcdUSB >> 8, dd.bcdUSB & 0xFF, dd.bDeviceClass,
            dd.bDeviceSubClass, dd.bDeviceProtocol, mps0, dd.idVendor, dd.idProduct,
            dd.bcdDevice >> 8, dd.bcdDevice & 0xFF, dd.bNumConfigurations);
    for (int off = 0; off + 2 <= d.config_len && d.config[off] >= 2; off += d.config[off]) {
        const uint8_t* p = d.config + off;
        if (p[1] == usb::DT_CONFIG) {
            const auto* c = reinterpret_cast<const usb::ConfigDescriptor*>(p);
            kprintf("  config %u: %u interface(s), total length %u, attributes 0x%02x, max power %u mA\n",
                    c->bConfigurationValue, c->bNumInterfaces, c->wTotalLength, c->bmAttributes, c->bMaxPower * 2u);
        } else if (p[1] == usb::DT_INTERFACE) {
            const auto* f = reinterpret_cast<const usb::InterfaceDescriptor*>(p);
            kprintf("  interface %u: class %u subclass %u protocol %u, %u endpoint(s)\n", f->bInterfaceNumber,
                    f->bInterfaceClass, f->bInterfaceSubClass, f->bInterfaceProtocol, f->bNumEndpoints);
        } else if (p[1] == usb::DT_ENDPOINT) {
            const auto* e = reinterpret_cast<const usb::EndpointDescriptor*>(p);
            static const char* const kinds[] = {"control", "isochronous", "bulk", "interrupt"};
            kprintf("  endpoint 0x%02x: %s %s, max packet %u, interval %u\n", e->bEndpointAddress,
                    kinds[e->bmAttributes & 3], (e->bEndpointAddress & 0x80) ? "IN" : "OUT",
                    e->wMaxPacketSize, e->bInterval);
        } else if (p[1] == usb::DT_HID) {
            kprintf("  HID descriptor: HID %x.%02x, report descriptor length %u\n", p[3], p[2], p[7] | (p[8] << 8));
        }
    }
}

uint8_t wait_port_change(uint32_t timeout_ms)
{
    for (int i = 0; i < g_nstash; ++i) {
        if (trb_type(g_stash[i]) == PORT_STATUS_CHANGE) {
            const uint8_t port = static_cast<uint8_t>(g_stash[i].param >> 24);
            for (int j = i + 1; j < g_nstash; ++j) g_stash[j - 1] = g_stash[j];
            --g_nstash;
            return port;
        }
    }
    Trb ev;
    const uint64_t end = clock::ms() + timeout_ms;
    while (clock::ms() < end) {
        if (!wait_event(ev, static_cast<uint32_t>(end - clock::ms()))) return 0;
        if (trb_type(ev) == PORT_STATUS_CHANGE) return static_cast<uint8_t>(ev.param >> 24);
    }
    return 0;
}
}  // namespace usbh
