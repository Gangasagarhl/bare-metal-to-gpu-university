// hid_kbd.cc - DR302 F4-09: boot keyboard reports -> characters.
#include "hid_kbd.h"
#include "../F4-01/kbase.h"
#include "../F4-08/intr.h"

namespace hidkbd {
char usage_to_ascii(uint8_t u, bool shift)
{
    if (u >= 0x04 && u <= 0x1D) return static_cast<char>((shift ? 'A' : 'a') + (u - 0x04));
    if (u >= 0x1E && u <= 0x26) return static_cast<char>('1' + (u - 0x1E));
    if (u == 0x27) return '0';
    if (u == 0x28) return '\n';                        // Enter
    if (u == 0x2C) return ' ';                         // Space
    if (u == 0x2D) return shift ? '_' : '-';
    if (u == 0x37) return shift ? '>' : '.';
    return 0;
}

bool start(usbh::Device& d)
{
    const usb::InterfaceDescriptor* f = usbh::interface(d, 0);
    if (!f || f->bInterfaceClass != usb::CLASS_HID || f->bInterfaceSubClass != 1 || f->bInterfaceProtocol != 1)
        return false;                                  // not a boot keyboard
    if (usbh::configure(d, f->bInterfaceNumber) < 1) return false;
    const uint8_t rt = usb::DIR_OUT | usb::TYPE_CLASS | usb::RECIP_INTERFACE;
    const int a = usbh::control(d, rt, REQ_SET_PROTOCOL, 0, f->bInterfaceNumber, nullptr, 0);  // 0 = boot
    const int b = usbh::control(d, rt, REQ_SET_IDLE, 0, f->bInterfaceNumber, nullptr, 0);      // report on change only
    kprintf("hid: SET_PROTOCOL(boot) -> %d, SET_IDLE(0) -> %d\n", a, b);
    return a >= 0 && b >= 0;
}

int read_line(usbh::Device& d, char* out, int max, uint32_t timeout_ms)
{
    usbh::Endpoint* ep = usbh::find_ep(d, usb::XFER_INT, true);
    if (!ep) return -1;
    uint8_t prev[8] = {}, rep[8];
    int n = 0;
    const uint64_t end = clock::ms() + timeout_ms;
    while (n < max - 1 && clock::ms() < end) {
        const int got = usbh::transfer(d, *ep, rep, 8, static_cast<uint32_t>(end - clock::ms()));
        if (got == -1000) break;                       // timeout
        if (got < 8) { kprintf("hid: short report (%d)\n", got); continue; }
        kprintf("hid: report %02x %02x %02x %02x %02x %02x %02x %02x\n", rep[0], rep[1], rep[2], rep[3],
                rep[4], rep[5], rep[6], rep[7]);
        const bool shift = rep[0] & 0x22;              // left or right Shift
        for (int k = 2; k < 8; ++k) {
            if (rep[k] < 4) continue;                  // 0 none, 1-3 error codes
            bool was_down = false;
            for (int j = 2; j < 8; ++j) was_down |= prev[j] == rep[k];
            if (was_down) continue;                    // still held: not a new press
            const char c = usage_to_ascii(rep[k], shift);
            if (c == '\n') { out[n] = 0; return n; }
            if (c) out[n++] = c;
        }
        memcpy(prev, rep, 8);
    }
    out[n] = 0;
    return n;
}
}  // namespace hidkbd
