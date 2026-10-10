// F1-41 Listing 3: x86 port I/O from C++ with GCC inline assembly.
// The out/in instructions use a separate 16-bit port address space. A normal
// user program is not allowed to execute them: run it and see what happens.
#include <cstdint>
#include <cstdio>

static inline void outb(std::uint16_t port, std::uint8_t value)
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline std::uint8_t inb(std::uint16_t port)
{
    std::uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

int main()
{
    std::printf("about to execute 'in' on port 0x3FD from user mode...\n");
    std::fflush(stdout);
    const std::uint8_t lsr = inb(0x3FD);
    std::printf("read 0x%02X\n", lsr);   // only reached if the CPU allowed it
    outb(0x80, 0);
    return 0;
}
