// uart_irq.h - DR301 F4-03: interface of the interrupt-driven UART driver.
#pragma once
#include <stdint.h>

struct UartStats {
    uint32_t interrupts, rx_bytes, tx_bytes, overruns, rx_dropped, rx_high_water;
};

void uart_irq_init(uint16_t base, uint8_t line);
bool uart_irq_getc(uint8_t& b);          // false if nothing has arrived
void uart_irq_putc(uint8_t b);           // queues the byte; sleeps if the ring is full
bool uart_irq_tx_idle();                 // everything sent and the transmitter is empty
const UartStats& uart_irq_stats();
// Diagnostics without side effects: IER, LSR and MCR (reading IIR could clear a cause).
void uart_irq_regs(uint8_t& ier, uint8_t& lsr, uint8_t& mcr);
