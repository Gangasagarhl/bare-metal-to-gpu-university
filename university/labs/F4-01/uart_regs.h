// uart_regs.h - DR301: the 16550 UART register map as reg.h types.
// Offsets from the port base; registers 0 and 1 change meaning when LCR.DLAB = 1.
// Written from the author's memory of the PC16550D datasheet (register description);
// checked against Linux's <linux/serial_reg.h> by regcheck.cpp (F4-01), not against the
// datasheet itself (see the unverified box in F4-01).
#pragma once
#include "reg.h"

namespace uart {
using RBR = Reg<uint8_t, 0, Access::RO>;   // receiver buffer (DLAB = 0, read)
using THR = Reg<uint8_t, 0, Access::WO>;   // transmitter holding (DLAB = 0, write)
using DLL = Reg<uint8_t, 0, Access::RW>;   // divisor latch, low byte (DLAB = 1)
using DLM = Reg<uint8_t, 1, Access::RW>;   // divisor latch, high byte (DLAB = 1)
using IER = Reg<uint8_t, 1, Access::RW>;   // interrupt enable (DLAB = 0)
using IIR = Reg<uint8_t, 2, Access::RO>;   // interrupt identification (read)
using FCR = Reg<uint8_t, 2, Access::WO>;   // FIFO control (write)
using LCR = Reg<uint8_t, 3, Access::RW>;   // line control
using MCR = Reg<uint8_t, 4, Access::RW>;   // modem control
using LSR = Reg<uint8_t, 5, Access::RO>;   // line status
using MSR = Reg<uint8_t, 6, Access::RO>;   // modem status
using SCR = Reg<uint8_t, 7, Access::RW>;   // scratch: no effect on the UART

using IIR_ID = Field<IIR, 1, 3>;           // which interrupt is pending (with bit 0 = 0)

constexpr uint8_t IER_RDI = 0x01;          // received data available
constexpr uint8_t IER_THRI = 0x02;         // transmitter holding register empty
constexpr uint8_t IIR_NO_INT = 0x01;       // bit 0 = 1: nothing pending
constexpr uint8_t IIR_RLSI = 0x06;         // (IIR & 0x0E) values: line status
constexpr uint8_t IIR_RDI = 0x04;          //   received data
constexpr uint8_t IIR_THRI = 0x02;         //   transmitter empty
constexpr uint8_t IIR_TIMEOUT = 0x0C;      //   character timeout (FIFO mode)
constexpr uint8_t FCR_ENABLE = 0x01, FCR_CLEAR_RX = 0x02, FCR_CLEAR_TX = 0x04;
constexpr uint8_t FCR_TRIGGER_14 = 0xC0;   // receive FIFO interrupt at 14 bytes
constexpr uint8_t LCR_8N1 = 0x03, LCR_DLAB = 0x80;
constexpr uint8_t MCR_DTR = 0x01, MCR_RTS = 0x02, MCR_OUT2 = 0x08, MCR_LOOP = 0x10;
constexpr uint8_t LSR_DR = 0x01, LSR_OE = 0x02, LSR_THRE = 0x20, LSR_TEMT = 0x40;
}  // namespace uart
