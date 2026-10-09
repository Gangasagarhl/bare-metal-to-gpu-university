// intr.h - IDT, the legacy PIC and the PIT: just enough for a periodic timer tick.
#pragma once
#include <stdint.h>

struct Frame {   // the order intr.S pushes them, lowest address first
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8, rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t vector, error, rip, cs, rflags, rsp, ss;
};

void idt_init();                       // exceptions report and exit; vector 32 = timer
void timer_start(unsigned hz);         // PIC remapped to 32..47, PIT channel 0 periodic
uint64_t ticks();                      // timer interrupts seen so far
uint16_t pit_read_counter();           // latch and read PIT channel 0 (the busy-poll way)
inline void irq_enable() { asm volatile("sti"); }
inline void irq_disable() { asm volatile("cli"); }
