// regcheck.cpp - DR301 F4-01: compares the UART constants of uart_regs.h (written from
// memory of the datasheet) with an independent source: Linux's <linux/serial_reg.h>,
// as installed in the build container. Agreement does not prove either is right, but a
// disagreement would prove one of them wrong.
#include <cstdio>
#include <linux/serial_reg.h>
#include "uart_regs.h"

static int failures = 0;

static void check(const char* name, unsigned ours, unsigned linux_value)
{
    const bool same = ours == linux_value;
    if (!same) ++failures;
    std::printf("%-14s ours 0x%02x  linux 0x%02x  %s\n", name, ours, linux_value, same ? "same" : "DIFFERENT");
}

int main()
{
    check("RBR offset", uart::RBR::offset, UART_RX);
    check("THR offset", uart::THR::offset, UART_TX);
    check("DLL offset", uart::DLL::offset, UART_DLL);
    check("DLM offset", uart::DLM::offset, UART_DLM);
    check("IER offset", uart::IER::offset, UART_IER);
    check("IIR offset", uart::IIR::offset, UART_IIR);
    check("FCR offset", uart::FCR::offset, UART_FCR);
    check("LCR offset", uart::LCR::offset, UART_LCR);
    check("MCR offset", uart::MCR::offset, UART_MCR);
    check("LSR offset", uart::LSR::offset, UART_LSR);
    check("MSR offset", uart::MSR::offset, UART_MSR);
    check("SCR offset", uart::SCR::offset, UART_SCR);
    check("IER_RDI", uart::IER_RDI, UART_IER_RDI);
    check("IER_THRI", uart::IER_THRI, UART_IER_THRI);
    check("IIR_NO_INT", uart::IIR_NO_INT, UART_IIR_NO_INT);
    check("IIR_RLSI", uart::IIR_RLSI, UART_IIR_RLSI);
    check("IIR_RDI", uart::IIR_RDI, UART_IIR_RDI);
    check("IIR_THRI", uart::IIR_THRI, UART_IIR_THRI);
    check("IIR_TIMEOUT", uart::IIR_TIMEOUT, UART_IIR_RX_TIMEOUT);
    check("FCR_ENABLE", uart::FCR_ENABLE, UART_FCR_ENABLE_FIFO);
    check("FCR_CLEAR_RX", uart::FCR_CLEAR_RX, UART_FCR_CLEAR_RCVR);
    check("FCR_CLEAR_TX", uart::FCR_CLEAR_TX, UART_FCR_CLEAR_XMIT);
    check("FCR_TRIGGER_14", uart::FCR_TRIGGER_14, UART_FCR_TRIGGER_14);
    check("LCR_DLAB", uart::LCR_DLAB, UART_LCR_DLAB);
    check("LCR_8N1", uart::LCR_8N1, UART_LCR_WLEN8);
    check("MCR_DTR", uart::MCR_DTR, UART_MCR_DTR);
    check("MCR_RTS", uart::MCR_RTS, UART_MCR_RTS);
    check("MCR_OUT2", uart::MCR_OUT2, UART_MCR_OUT2);
    check("MCR_LOOP", uart::MCR_LOOP, UART_MCR_LOOP);
    check("LSR_DR", uart::LSR_DR, UART_LSR_DR);
    check("LSR_OE", uart::LSR_OE, UART_LSR_OE);
    check("LSR_THRE", uart::LSR_THRE, UART_LSR_THRE);
    check("LSR_TEMT", uart::LSR_TEMT, UART_LSR_TEMT);
    std::printf("%d difference(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
