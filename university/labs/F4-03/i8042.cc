// i8042.cc - DR301 F4-03: i8042 PS/2 controller with keyboard (line 1) and mouse (line 12).
// Command and status values written from the author's memory of the OSDev wiki pages
// "8042 PS/2 Controller", "PS/2 Keyboard" and "PS/2 Mouse" (the curriculum's sources for
// C2); confirmed only by QEMU's i8042 model answering as expected (see F4-03).
#include "i8042.h"
#include "irq.h"
#include "kbase.h"
#include "ring.h"
#include "../F4-01/driver.h"

namespace {
constexpr uint16_t DATA = 0x60, STATUS = 0x64, CMD = 0x64;
constexpr uint8_t ST_OUT_FULL = 0x01, ST_IN_FULL = 0x02, ST_AUX = 0x20;

Ring<256> g_kbd;                          // raw scan codes from the keyboard
Ring<256> g_aux;                          // raw bytes from the mouse
bool g_shift = false;
uint8_t g_pkt[3];
int g_pkt_n = 0;
uint32_t g_resyncs = 0;

bool wait_in_empty()
{
    for (int i = 0; i < 1000000; ++i) if (!(inb(STATUS) & ST_IN_FULL)) return true;
    return false;
}
bool wait_out_full()
{
    for (int i = 0; i < 1000000; ++i) if (inb(STATUS) & ST_OUT_FULL) return true;
    return false;
}
bool command(uint8_t c) { if (!wait_in_empty()) return false; outb(CMD, c); return true; }
bool write_data(uint8_t d) { if (!wait_in_empty()) return false; outb(DATA, d); return true; }
int read_data() { return wait_out_full() ? inb(DATA) : -1; }
void flush() { while (inb(STATUS) & ST_OUT_FULL) (void)inb(DATA); }

// Send a byte to a device (port 2 = mouse goes through command 0xD4) and expect ACK 0xFA.
bool device_cmd(bool mouse, uint8_t b)
{
    if (mouse && !command(0xD4)) return false;
    if (!write_data(b)) return false;
    return read_data() == 0xFA;
}

void kbd_isr() { if (inb(STATUS) & ST_OUT_FULL) g_kbd.push(inb(DATA)); }
void aux_isr() { if (inb(STATUS) & ST_OUT_FULL) g_aux.push(inb(DATA)); }

// Scan code set 1 (what the controller delivers with translation on), make codes only.
constexpr char kLower[0x3A] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '};
constexpr char kUpper[0x3A] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0, 'A', 'S',
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|', 'Z', 'X', 'C', 'V',
    'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '};
constexpr uint8_t LSHIFT = 0x2A, RSHIFT = 0x36;
}  // namespace

namespace ps2 {
int init()
{
    command(0xAD);                        // disable port 1 (keyboard)
    command(0xA7);                        // disable port 2 (mouse)
    flush();
    command(0x20);                        // read the configuration byte
    int cfg = read_data();
    if (cfg < 0) return -E_TIMEDOUT;
    kprintf("i8042: config byte 0x%02x (translation %s)\n", cfg, (cfg & 0x40) ? "on" : "off");
    cfg &= ~0x03;                         // no interrupts while we test
    command(0x60); write_data(static_cast<uint8_t>(cfg));
    command(0xAA);                        // controller self-test
    int r = read_data();
    kprintf("i8042: self-test 0x%02x\n", r);
    if (r != 0x55) return -E_IO;
    command(0x60); write_data(static_cast<uint8_t>(cfg));   // some controllers reset it
    command(0xAB);                        // test port 1
    const int t1 = read_data();
    command(0xA9);                        // test port 2
    const int t2 = read_data();
    kprintf("i8042: port tests %d %d (0 = passed)\n", t1, t2);
    command(0xAE);                        // enable port 1
    command(0xA8);                        // enable port 2
    // reset both devices: ACK 0xFA, then 0xAA "self-test passed" (the mouse adds its ID 0x00)
    const bool k_ack = device_cmd(false, 0xFF);
    const int k_bat = read_data();
    const bool m_ack = device_cmd(true, 0xFF);
    const int m_bat = read_data();
    const int m_id = read_data();
    kprintf("i8042: keyboard reset ack=%d result 0x%02x; mouse reset ack=%d result 0x%02x id 0x%02x\n",
            k_ack, k_bat, m_ack, m_bat, m_id);
    const bool m_on = device_cmd(true, 0xF4);   // mouse: enable data reporting
    kprintf("i8042: mouse data reporting %s\n", m_on ? "on" : "FAILED");
    flush();
    irq::set_handler(1, kbd_isr);
    irq::set_handler(12, aux_isr);
    irq::unmask(1);
    irq::unmask(2);                       // the cascade: lines 8-15 arrive through line 2
    irq::unmask(12);
    // Enabling the ports cleared their "clock disabled" bits (4 and 5) in the configuration
    // byte: read it again rather than writing back the stale copy, then turn on interrupts.
    command(0x20);
    cfg = read_data();
    command(0x60); write_data(static_cast<uint8_t>((cfg | 0x03) & ~0x30));
    kprintf("i8042: config byte now 0x%02x\n", (cfg | 0x03) & ~0x30);
    return 0;
}

bool get_key(KeyEvent& e)
{
    uint8_t sc;
    while (g_kbd.pop(sc)) {
        if (sc == 0xE0) continue;         // extended prefix: not handled in this lab
        const bool pressed = !(sc & 0x80);
        const uint8_t code = sc & 0x7F;
        if (code == LSHIFT || code == RSHIFT) { g_shift = pressed; continue; }
        e.code = code;
        e.pressed = pressed;
        e.ascii = (code < sizeof(kLower)) ? (g_shift ? kUpper[code] : kLower[code]) : 0;
        return true;
    }
    return false;
}

bool get_mouse(MouseEvent& e)
{
    uint8_t b;
    while (g_aux.pop(b)) {
        // byte 0 of every packet has bit 3 set: use it to find the start again after a loss
        if (g_pkt_n == 0 && !(b & 0x08)) { ++g_resyncs; continue; }
        g_pkt[g_pkt_n++] = b;
        if (g_pkt_n < 3) continue;
        g_pkt_n = 0;
        int dx = g_pkt[1], dy = g_pkt[2];
        if (g_pkt[0] & 0x10) dx -= 256;    // 9-bit two's complement: sign bits in byte 0
        if (g_pkt[0] & 0x20) dy -= 256;
        e.dx = dx;
        e.dy = dy;
        e.buttons = g_pkt[0] & 0x07;
        return true;
    }
    return false;
}

uint32_t resyncs() { return g_resyncs; }
}  // namespace ps2
