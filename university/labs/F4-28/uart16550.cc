// uart16550.cc - F4-28: F3-18's serial.h interface for the 16550-compatible UART that QEMU's
// RISC-V virt machine lists as "ns16550a". Same register model as the PC's COM1 (DR301,
// F4-03), but reached through memory instead of I/O ports, at the address the devicetree
// gives, with registers spaced 1 << reg-shift bytes apart. Register numbers after the
// 16550 datasheet as used in F4-03 (title only, pending verification).
#include <cstdint>
#include "serial.h"
#include "uart16550.h"

namespace {
volatile uint8_t* g_base = nullptr;
uint32_t g_shift = 0;

constexpr uint32_t kRBR = 0, kTHR = 0, kIER = 1, kLSR = 5;
constexpr uint8_t kLSR_DR = 1u << 0;     // receive data ready
constexpr uint8_t kLSR_THRE = 1u << 5;   // transmit holding register empty

volatile uint8_t& reg(uint32_t r)
{
    return g_base[r << g_shift];
}
} // namespace

namespace uart16550 {

void set_base(uint64_t phys, uint32_t reg_shift)
{
    g_base = reinterpret_cast<volatile uint8_t*>(phys);
    g_shift = reg_shift;
}

uint64_t base()
{
    return reinterpret_cast<uint64_t>(g_base);
}

bool read_char(char* c)
{
    if (g_base == nullptr || (reg(kLSR) & kLSR_DR) == 0) {
        return false;
    }
    *c = static_cast<char>(reg(kRBR));
    return true;
}

void enable_rx_interrupt()
{
    if (g_base != nullptr) {
        reg(kIER) = 0x01;                // "received data available" interrupt only
    }
}

} // namespace uart16550

namespace serial {

void init() {}   // QEMU's model needs no set-up; a board's UART does (F4-29, untested)

void put(char c)
{
    if (g_base == nullptr) {
        return;
    }
    if (c == '\n') {
        put('\r');
    }
    while ((reg(kLSR) & kLSR_THRE) == 0) {
    }
    reg(kTHR) = static_cast<uint8_t>(c);
}

void write(const char* s)
{
    while (*s != '\0') {
        put(*s++);
    }
}

} // namespace serial
