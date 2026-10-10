// reg_ro_write.cpp - DR301 F4-01: this file must NOT compile. A driver tries to write the
// read-only Line Status Register; reg.h turns the mistake into a compile-time error.
#include <cstdint>
#include <map>
#include "reg.h"
#include "uart_regs.h"

struct NullBus {
    template <typename T> T read(uint32_t) const { return T{}; }
    template <typename T> void write(uint32_t, T) const {}
};

int main()
{
    RegBlock<NullBus> u{NullBus{}};
    u.write<uart::LSR>(0x00);    // wrong: LSR is read-only (Access::RO)
    return 0;
}
