// uart16550.cc - DR301 F4-01: a polled 16550 UART driver written against the driver model.
// probe() proves the device is there before claiming it; remove() puts it back to rest.
#include "driver.h"
#include "kbase.h"
#include "portbus.h"
#include "uart_regs.h"

namespace {
using Uart = RegBlock<PortBus>;

struct UartState {
    Uart regs{PortBus{0}};
    bool in_use = false;
};
UartState g_uart[4];                     // up to four ports (COM1..COM4)

int uart_probe(Device& d)
{
    Uart u{PortBus{d.io_base}};
#ifndef F401_SKIP_PRESENCE_CHECK
    // 1. Scratch register: a real 16550 stores what we write; an empty I/O address does not.
    u.write<uart::SCR>(0x5A);
    if (u.read<uart::SCR>() != 0x5A) return -E_NODEV;
    u.write<uart::SCR>(0xA5);
    if (u.read<uart::SCR>() != 0xA5) return -E_NODEV;
    // 2. Loopback: transmitter wired to receiver inside the chip; nothing leaves the port.
    const uint8_t old_mcr = u.read<uart::MCR>();
    u.write<uart::MCR>(uart::MCR_LOOP);
    while (u.read<uart::LSR>() & uart::LSR_DR) (void)u.read<uart::RBR>();   // drain
    u.write<uart::THR>(0xAE);
    for (int i = 0; i < 100000 && !(u.read<uart::LSR>() & uart::LSR_DR); ++i) { }
    const bool echoed = (u.read<uart::LSR>() & uart::LSR_DR) && u.read<uart::RBR>() == 0xAE;
    u.write<uart::MCR>(old_mcr);
    if (!echoed) return -E_IO;
#endif
    // 3. Initialise: 115200 baud (divisor 1), 8 data bits, no parity, 1 stop bit, FIFOs on.
    u.write<uart::IER>(0x00);
    u.write<uart::LCR>(uart::LCR_DLAB);
    u.write<uart::DLL>(0x01);
    u.write<uart::DLM>(0x00);
    u.write<uart::LCR>(uart::LCR_8N1);
    u.write<uart::FCR>(uart::FCR_ENABLE | uart::FCR_CLEAR_RX | uart::FCR_CLEAR_TX);
    u.write<uart::MCR>(uart::MCR_DTR | uart::MCR_RTS);
    for (auto& s : g_uart) {
        if (!s.in_use) {
            s.regs = u;
            s.in_use = true;
            d.priv = &s;
            return 0;
        }
    }
    return -E_BUSY;
}

void uart_remove(Device& d)
{
    auto* s = static_cast<UartState*>(d.priv);
    s->regs.write<uart::IER>(0x00);      // no interrupts from a device nobody owns
    s->in_use = false;
}

constexpr MatchId uart_ids[] = {
    {BusKind::Platform, "ns16550", 0xFFFF, 0xFFFF, 0, 0},
};
}  // namespace

Driver uart16550_driver = {"uart16550", uart_ids, 1, uart_probe, uart_remove};

// The service the driver offers upward: write a string to the port owned by 'd'.
void uart_write(Device& d, const char* s)
{
    auto* st = static_cast<UartState*>(d.priv);
    for (; *s; ++s) {
        if (*s == '\n') {
            while (!(st->regs.read<uart::LSR>() & uart::LSR_THRE)) { }
            st->regs.write<uart::THR>('\r');
        }
        while (!(st->regs.read<uart::LSR>() & uart::LSR_THRE)) { }
        st->regs.write<uart::THR>(static_cast<uint8_t>(*s));
    }
}

// Diagnostics: one snapshot of the status registers of the port owned by 'd'.
void uart_dump(Device& d)
{
    auto* st = static_cast<UartState*>(d.priv);
    kprintf("  %s: LSR=0x%02x IIR=0x%02x MCR=0x%02x SCR=0x%02x\n", d.name,
            st->regs.read<uart::LSR>(), st->regs.read<uart::IIR>(),
            st->regs.read<uart::MCR>(), st->regs.read<uart::SCR>());
}
