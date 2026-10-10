// regs_codegen.cpp: compiled, not run. The disassembly shows what the compiler does with
// plain and volatile accesses, and with a bit-field in a register layout.
#include <cstdint>

struct CtrlBits {               // a "register" described with bit-fields
    std::uint32_t enable : 1;
    std::uint32_t mode : 3;
    std::uint32_t reserved : 28;
};

void setModeBitfield(volatile CtrlBits* c)
{
    c->mode = 5;
}

void setModeMask(volatile std::uint32_t* c)    // the same change with an explicit mask
{
    std::uint32_t v = *c;                      // one 32-bit read
    v = (v & ~(0x7u << 1)) | (5u << 1);
    *c = v;                                    // one 32-bit write
}

void writeTwicePlain(std::uint32_t* r)
{
    *r = 1;
    *r = 2;
}

void writeTwiceVolatile(volatile std::uint32_t* r)
{
    *r = 1;
    *r = 2;
}

std::uint32_t pollPlain(const std::uint32_t* status)
{
    while (*status & 1) {
    }
    return *status;
}

std::uint32_t pollVolatile(const volatile std::uint32_t* status)
{
    while (*status & 1) {
    }
    return *status;
}
