// hooks.h - entry points that the core (F3-26 files) calls in code from later chapters.
// The OS303 kernel is built from all five lab folders together; see build_kernel.sh.
#pragma once
#include "cpu.h"

namespace k {
struct Process;
// F3-28 (smp.cc, acpi.cc)
void smp_start_aps();
void tlb_shootdown(uint64_t va, uint64_t pages);   // flush on every online CPU
void tlb_ipi_handler();
void lapic_send_ipi(uint32_t apic_id, uint8_t vector);
// F3-29 (syscall.cc, uaccess.S)
void syscall_init_cpu();
bool uaccess_fixup(TrapFrame* f);                   // fault inside copy_from/to_user?
// F3-30 (process.cc)
bool vm_handle_fault(TrapFrame* f, uint64_t cr2);   // demand paging of user memory
[[noreturn]] void user_fault(TrapFrame* f, uint64_t cr2);
void user_return_check(TrapFrame* f);               // kill requests before returning to ring 3
uint64_t proc_cr3(Process* p);
uint64_t kernel_cr3();
// lapic.cc (this folder)
void pic_disable();
void lapic_init_cpu();
uint32_t lapic_id();
void lapic_eoi();
void lapic_calibrate();
void lapic_timer_start(int hz);
uint64_t tsc_per_ms();
void pit_wait_us(uint32_t us);
// Timer-interrupt hook used by one B10 test (mutex taken in an interrupt handler).
extern void (*volatile timer_hook)();
}  // namespace k
