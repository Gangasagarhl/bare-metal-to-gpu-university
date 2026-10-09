// uart_any.cc - F4-29: F3-18's serial.h interface for the two UARTs this course meets on
// RISC-V: the 16550-compatible "ns16550a" of QEMU virt (as in F4-28) and the "sifive,uart0"
// of the SiFive boards (QEMU's sifive_u models the HiFive Unleashed). The devicetree's
// compatible string picks the driver. sifive,uart0 register offsets and bits after the
// SiFive FU540-C000 Manual, UART chapter (title only, pending verification); the sifive_u
// run in the lab shows QEMU's model accepting them.
#include <cstdint>
#include "serial.h"
#include "uart_any.h"

namespace {
enum class Kind { None, Ns16550, Sifive };
Kind g_kind = Kind::None;
uint64_t g_base = 0;
uint32_t g_shift = 0;

volatile uint8_t& r8(uint32_t r) { return *reinterpret_cast<volatile uint8_t*>(g_base + (r << g_shift)); }
volatile uint32_t& r32(uint32_t off) { return *reinterpret_cast<volatile uint32_t*>(g_base + off); }

// ns16550a
constexpr uint32_t kRBR = 0, kTHR = 0, kIER = 1, kLSR = 5;
constexpr uint8_t kLSR_DR = 1u << 0, kLSR_THRE = 1u << 5;
// sifive,uart0
constexpr uint32_t kTxData = 0x00, kRxData = 0x04, kTxCtrl = 0x08, kRxCtrl = 0x0c;
constexpr uint32_t kFull = 1u << 31, kEmpty = 1u << 31;
} // namespace

namespace uart {

bool init_from(const fdt::Blob& dt, const fdt::Node& n)
{
    uint64_t size = 0;
    if (!dt.reg(n, 0, &g_base, &size)) {
        return false;
    }
    if (dt.is_compatible(n, "ns16550a")) {
        fdt::Prop p;
        g_shift = dt.get_prop(n, "reg-shift", &p) ? fdt::be32(p.data) : 0;
        g_kind = Kind::Ns16550;
        return true;
    }
    if (dt.is_compatible(n, "sifive,uart0")) {
        r32(kTxCtrl) = r32(kTxCtrl) | 1;   // txen (firmware normally left it on)
        r32(kRxCtrl) = r32(kRxCtrl) | 1;   // rxen
        g_kind = Kind::Sifive;
        return true;
    }
    return false;
}

const char* kind()
{
    return g_kind == Kind::Ns16550 ? "ns16550a" : g_kind == Kind::Sifive ? "sifive,uart0" : "none";
}

uint64_t base()
{
    return g_base;
}

bool read_char(char* c)
{
    if (g_kind == Kind::Ns16550 && (r8(kLSR) & kLSR_DR) != 0) {
        *c = static_cast<char>(r8(kRBR));
        return true;
    }
    if (g_kind == Kind::Sifive) {
        uint32_t v = r32(kRxData);
        if ((v & kEmpty) == 0) {
            *c = static_cast<char>(v & 0xff);
            return true;
        }
    }
    return false;
}

void enable_rx_interrupt()
{
    if (g_kind == Kind::Ns16550) {
        r8(kIER) = 0x01;
    }
}

} // namespace uart

namespace serial {

void init() {}

void put(char c)
{
    if (c == '\n') {
        put('\r');
    }
    if (g_kind == Kind::Ns16550) {
        while ((r8(kLSR) & kLSR_THRE) == 0) {
        }
        r8(kTHR) = static_cast<uint8_t>(c);
    } else if (g_kind == Kind::Sifive) {
        while ((r32(kTxData) & kFull) != 0) {
        }
        r32(kTxData) = static_cast<uint8_t>(c);
    }
}

void write(const char* s)
{
    while (*s != '\0') {
        put(*s++);
    }
}

} // namespace serial
