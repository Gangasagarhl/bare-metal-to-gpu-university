// pl011.cc - F4-24: the serial interface of F3-18 (serial.h, unchanged) implemented for an
// Arm PrimeCell PL011 UART. The base address comes from the devicetree (d1_main.cc), never
// from a constant. Register offsets and bits after the "PrimeCell UART (PL011) Technical
// Reference Manual" (Arm; title only, pending verification).
#include <cstdint>
#include "pl011.h"
#include "serial.h"

namespace {
volatile uint32_t* g_base = nullptr;     // nullptr: no console yet, output is dropped

constexpr uint32_t kDR = 0x00 / 4;       // data register
constexpr uint32_t kFR = 0x18 / 4;       // flag register
constexpr uint32_t kFR_TXFF = 1u << 5;   // transmit FIFO full
constexpr uint32_t kFR_RXFE = 1u << 4;   // receive FIFO empty
constexpr uint32_t kIMSC = 0x38 / 4;     // interrupt mask set/clear
constexpr uint32_t kICR = 0x44 / 4;      // interrupt clear
constexpr uint32_t kRXIM = 1u << 4;      // receive interrupt mask bit
constexpr uint32_t kRTIM = 1u << 6;      // receive timeout interrupt mask bit
} // namespace

namespace pl011 {

void set_base(uint64_t phys)
{
    g_base = reinterpret_cast<volatile uint32_t*>(phys);
}

uint64_t base()
{
    return reinterpret_cast<uint64_t>(g_base);
}

bool read_char(char* c)
{
    if (g_base == nullptr || (g_base[kFR] & kFR_RXFE) != 0) {
        return false;
    }
    *c = static_cast<char>(g_base[kDR] & 0xff);
    return true;
}

void enable_rx_interrupt()
{
    if (g_base == nullptr) {
        return;
    }
#ifdef MISTAKE_CLEAR_RX_STATUS                     // F4-25 "common mistakes" build only
    g_base[kICR] = 0x7ff;                          // clears "data waiting" too
#else
    g_base[kICR] = 0x7ff & ~(kRXIM | kRTIM);       // clear stale status, but not "data waiting":
#endif
    g_base[kIMSC] = kRXIM | kRTIM;                 // a character that is already here must still interrupt
}

} // namespace pl011

namespace serial {

// QEMU's PL011 needs no set-up. A real board's firmware may leave the UART disabled or at
// another baud rate: see the F4-27 lab for the initialisation a board needs (untested).
void init() {}

void put(char c)
{
    if (g_base == nullptr) {
        return;
    }
    if (c == '\n') {
        put('\r');
    }
    while ((g_base[kFR] & kFR_TXFF) != 0) {
    }
    g_base[kDR] = static_cast<uint8_t>(c);
}

void write(const char* s)
{
    while (*s != '\0') {
        put(*s++);
    }
}

} // namespace serial
