// cpu.cc - GDT, TSS and IDT setup for each CPU (Intel SDM Vol. 3, "Protected-Mode
// Memory Management" and "Interrupt and Exception Handling"; title only).
#include "cpu.h"

namespace k {

namespace {
Cpu cpus[kMaxCpus];
int ncpus = 0;

struct __attribute__((packed)) IdtGate {
    uint16_t off_lo, sel;
    uint8_t ist, type;           // type 0x8E: present, DPL 0, 64-bit interrupt gate
    uint16_t off_mid;
    uint32_t off_hi, zero;
};
IdtGate idt[256];

struct __attribute__((packed)) TablePtr {
    uint16_t limit;
    uint64_t base;
};
}  // namespace

extern "C" uint64_t trap_stubs[256];     // trap.S: address of each vector's entry stub

Cpu& cpu_by_index(int i) { return cpus[i]; }
int cpu_count() { return ncpus; }

void idt_init()
{
    for (int v = 0; v < 256; ++v) {
        uint64_t a = trap_stubs[v];
        idt[v] = IdtGate{uint16_t(a), kKernelCS, 0, 0x8E, uint16_t(a >> 16), uint32_t(a >> 32), 0};
    }
    idt[8].ist = 1;   // double fault runs on its own known-good stack (IST1)
}

void cpu_setup(Cpu& c, int id, uint32_t apic_id)
{
    c.self = &c;
    c.id = id;
    c.apic_id = apic_id;
    c.gdt[0] = 0;
    c.gdt[1] = 0x00AF9A000000FFFF;   // 0x08 kernel code: present, DPL 0, L=1
    c.gdt[2] = 0x00CF92000000FFFF;   // 0x10 kernel data
    c.gdt[3] = 0x00CFF2000000FFFF;   // 0x18 user data, DPL 3
    c.gdt[4] = 0x00AFFA000000FFFF;   // 0x20 user code, DPL 3, L=1
    uint64_t t = reinterpret_cast<uint64_t>(&c.tss);
    uint64_t lim = sizeof(Tss) - 1;
    c.gdt[5] = (lim & 0xFFFF) | ((t & 0xFFFFFF) << 16) | (0x89ull << 40) |  // 64-bit TSS, available
               (((lim >> 16) & 0xF) << 48) | (((t >> 24) & 0xFF) << 56);
    c.gdt[6] = t >> 32;
    c.tss.iomap_base = sizeof(Tss);
    c.tss.ist[0] = reinterpret_cast<uint64_t>(c.df_stack + sizeof(c.df_stack));

    TablePtr g{sizeof(c.gdt) - 1, reinterpret_cast<uint64_t>(c.gdt)};
    asm volatile("lgdt %0" : : "m"(g));
    // Reload CS with a far return, then the data segments and the task register.
    asm volatile("pushq %0; leaq 1f(%%rip), %%rax; pushq %%rax; lretq; 1:"
                 : : "i"(uint64_t(kKernelCS)) : "rax", "memory");
    asm volatile("mov %0, %%ds; mov %0, %%es; mov %0, %%ss" : : "r"(uint32_t(kKernelDS)));
    asm volatile("ltr %0" : : "r"(uint16_t(kTssSel)));
    TablePtr i{sizeof(idt) - 1, reinterpret_cast<uint64_t>(idt)};
    asm volatile("lidt %0" : : "m"(i));
    // Loading FS/GS selectors would clear the bases, so set the selectors first.
    asm volatile("mov %0, %%fs; mov %0, %%gs" : : "r"(0));
    wrmsr(0xC0000101, reinterpret_cast<uint64_t>(&c));   // IA32_GS_BASE = &cpu
    wrmsr(0xC0000102, 0);                                 // IA32_KERNEL_GS_BASE (user GS)
    if (id >= ncpus) {
        ncpus = id + 1;
    }
}

}  // namespace k
