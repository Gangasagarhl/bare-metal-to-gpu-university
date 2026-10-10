// e1000.h - DS403 cluster kernel: a polled driver for QEMU's e1000 (Intel 82540EM model),
// the NIC of curriculum milestone C9. One receive ring, one transmit ring, no interrupts.
#pragma once
#include <stdint.h>

bool e1000_init();                                     // find, reset and start the NIC
const uint8_t* e1000_mac();                            // the 6-byte MAC address
bool e1000_send(const void* frame, uint16_t len);      // copies the frame; false if ring full
int e1000_poll(void* buf, uint16_t cap);               // one received frame's length, or 0
