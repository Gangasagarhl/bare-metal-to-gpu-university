// F1-41 Listing 2: two ways to wait for a status bit. Compiled with -O2 and
// disassembled (not run): the question is what loads the compiler keeps.
#include <cstdint>

void wait_ready_plain(const std::uint32_t* status)
{
    while ((*status & 1u) == 0) {
        // spin until the device sets bit 0
    }
}

void wait_ready_volatile(const volatile std::uint32_t* status)
{
    while ((*status & 1u) == 0) {
        // spin until the device sets bit 0
    }
}
