// HW204 midterm, question M6 evidence (Lab Engineer). Two wait loops on a device status
// word, both taking the same plain pointer; one of them goes through a volatile pointer
// inside. Compiled at -O2 (no sanitizers) and disassembled by run.sh; candidates receive
// only the disassembly and must say which function really waits. Compiled, not run.
#include <cstdint>

std::uint32_t wait_a(const std::uint32_t* status)
{
    while ((*status & 0x1u) == 0) {
    }
    return *status;
}

std::uint32_t wait_b(const std::uint32_t* status)
{
    const volatile std::uint32_t* s = status;
    while ((*s & 0x1u) == 0) {
    }
    return *s;
}
