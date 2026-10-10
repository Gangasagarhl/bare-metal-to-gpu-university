// arch_x86.cc - BR-06: the x86 arch layer of the probe (QEMU q35, 32-bit Multiboot entry).
#include "probe.h"

namespace {

void outb(unsigned short port, u8 v)
{
    asm volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}

void outl(unsigned short port, u32 v)
{
    asm volatile("outl %0, %1" : : "a"(v), "Nd"(port));
}

} // namespace

const char* arch::name()
{
    return "x86 (QEMU q35, 32-bit Multiboot entry)";
}

void arch::putc(char c)
{
    outb(0x3f8, static_cast<u8>(c));   // COM1 transmit register: port I/O, a PC fixture
}

[[noreturn]] void arch::exit(bool ok)
{
    // isa-debug-exit device at port 0xf4: QEMU exits with status (value << 1) | 1.
    outl(0xf4, ok ? 0 : 1);
    for (;;) {
        asm volatile("hlt");
    }
}

extern "C" [[noreturn]] void arch_entry(u32 eax, u32 ebx, u32 cs, u32 cr0)
{
    EntryState st{};
    st.reg_name[0] = "eax (Multiboot magic) ";
    st.reg_name[1] = "ebx (Multiboot info)  ";
    st.reg_name[2] = "cr0                   ";
    st.reg_value[0] = eax;
    st.reg_value[1] = ebx;
    st.reg_value[2] = cr0;
    st.nregs = 3;
    st.dtb = nullptr;
    st.privilege = (cs & 3) == 0 ? "ring 0" : "ring 1-3 (unexpected)";
    st.how = "low two bits of the CS selector";
    probe_main(st);
}
