// uart_irq.cc - DR301 F4-03: an interrupt-driven 16550 driver with receive and transmit
// rings. The interrupt handler moves bytes between the chip's FIFOs and the rings; normal
// code only touches the rings.
#include "../F4-01/portbus.h"
#include "../F4-01/uart_regs.h"
#include "irq.h"
#include "ring.h"
#include "uart_irq.h"

namespace {
RegBlock<PortBus> g_u{PortBus{0}};
Ring<4096> g_rx, g_tx;
volatile bool g_tx_running = false;      // is the THRE interrupt enabled right now?
UartStats g_stats;

void uart_isr()
{
    ++g_stats.interrupts;
    for (;;) {                           // several causes can be pending: loop until none
        const uint8_t iir = g_u.read<uart::IIR>();
        if (iir & uart::IIR_NO_INT) break;
        switch (iir & 0x0E) {
        case uart::IIR_RLSI:             // line status: reading LSR clears it
            if (g_u.read<uart::LSR>() & uart::LSR_OE) ++g_stats.overruns;
            break;
        case uart::IIR_RDI:
        case uart::IIR_TIMEOUT:          // data waiting (FIFO at trigger level, or idle)
            while (g_u.read<uart::LSR>() & uart::LSR_DR) {
                g_rx.push(g_u.read<uart::RBR>());
                ++g_stats.rx_bytes;
            }
            g_rx.note_level();
            break;
        case uart::IIR_THRI: {           // transmitter empty: refill up to 16 bytes (FIFO)
            uint8_t b;
            int n = 0;
            while (n < 16 && g_tx.pop(b)) { g_u.write<uart::THR>(b); ++n; ++g_stats.tx_bytes; }
            if (n == 0) {                // nothing left: stop asking for THRE interrupts
                g_u.write<uart::IER>(uart::IER_RDI);
                g_tx_running = false;
            }
            break;
        }
        default:                         // modem status: reading MSR clears it
            (void)g_u.read<uart::MSR>();
            break;
        }
    }
}
}  // namespace

void uart_irq_init(uint16_t base, uint8_t line)
{
    g_u = RegBlock<PortBus>{PortBus{base}};
    g_u.write<uart::IER>(0);
    g_u.write<uart::LCR>(uart::LCR_DLAB);
    g_u.write<uart::DLL>(1);
    g_u.write<uart::DLM>(0);
    g_u.write<uart::LCR>(uart::LCR_8N1);
    g_u.write<uart::FCR>(uart::FCR_ENABLE | uart::FCR_CLEAR_RX | uart::FCR_CLEAR_TX | uart::FCR_TRIGGER_14);
    // OUT2 connects the UART's interrupt output to the PIC line on PC serial cards
    g_u.write<uart::MCR>(uart::MCR_DTR | uart::MCR_RTS | uart::MCR_OUT2);
    irq::set_handler(line, uart_isr);
    irq::unmask(line);
    g_u.write<uart::IER>(uart::IER_RDI);  // receive interrupts on from now on
}

bool uart_irq_getc(uint8_t& b) { return g_rx.pop(b); }

void uart_irq_putc(uint8_t b)
{
    while (g_tx.size() == 4096) irq::wait();   // ring full: sleep until the ISR drains some
    g_tx.push(b);
    irq::disable();                      // the ISR also changes g_tx_running and IER
    if (!g_tx_running) {
        g_tx_running = true;
        g_u.write<uart::IER>(uart::IER_RDI | uart::IER_THRI);   // THRE fires when enabled
    }
    irq::enable();
}

bool uart_irq_tx_idle()
{
    return !g_tx_running && (g_u.read<uart::LSR>() & uart::LSR_TEMT);
}

const UartStats& uart_irq_stats()
{
    g_stats.rx_dropped = g_rx.dropped();
    g_stats.rx_high_water = g_rx.high_water();
    return g_stats;
}

void uart_irq_regs(uint8_t& ier, uint8_t& lsr, uint8_t& mcr)
{
    ier = g_u.read<uart::IER>();
    lsr = g_u.read<uart::LSR>();          // note: reading LSR clears its error bits
    mcr = g_u.read<uart::MCR>();
}
