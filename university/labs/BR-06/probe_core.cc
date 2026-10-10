// probe_core.cc - BR-06: the arch-neutral part of the entry probe. Compiled unchanged for
// all three machines. It never asks "which CPU am I?": it prints what the arch layer found.
#include "probe.h"

namespace {

void put(const char* s)
{
    while (*s != '\0') {
        arch::putc(*s++);
    }
}

void hex(u64 v, int digits)
{
    put("0x");
    for (int i = digits - 1; i >= 0; --i) {
        arch::putc("0123456789abcdef"[(v >> (4 * i)) & 0xf]);
    }
}

// A devicetree blob stores every number big-endian, whatever the CPU (F4-23).
u32 be32(const u8* p)
{
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}

// The same four bytes read in the CPU's own byte order: the endianness trap.
u32 native32(const u8* p)
{
    u32 v;
    __builtin_memcpy(&v, p, sizeof v);
    return v;
}

} // namespace

extern "C" [[noreturn]] void probe_main(const EntryState& st)
{
    put("BR-06 entry probe on ");
    put(arch::name());
    put("\n  privilege at entry: ");
    put(st.privilege);
    put("  (");
    put(st.how);
    put(")\n");
    for (int i = 0; i < st.nregs; ++i) {
        put("  ");
        put(st.reg_name[i]);
        put(" = ");
        hex(st.reg_value[i], 16);
        put("\n");
    }
    if (st.dtb != nullptr) {
        put("  hardware description: devicetree\n");
        put("  DTB magic, explicit big-endian read: ");
        hex(be32(st.dtb), 8);
        put("\n  DTB magic, native-order read:        ");
        hex(native32(st.dtb), 8);
        put("\n");
    } else {
        put("  hardware description: none passed in registers (firmware tables instead)\n");
    }
#ifdef ASSUME_APIC
    // The trap: "core" code that knows where a PC keeps its local APIC.
    put("  core code reads the local APIC version register at 0xfee00030 ...\n");
    const u32 v = *reinterpret_cast<volatile u32*>(uptr{0xfee00030});
    put("  APIC version register = ");
    hex(v, 8);
    put("\n");
#endif
    put("probe done\n");
    arch::exit(true);
}

extern "C" [[noreturn]] void probe_fatal(const char* what, u64 cause, u64 addr, u64 pc)
{
    put("  EXCEPTION: ");
    put(what);
    put("\n    cause ");
    hex(cause, 16);
    put("  address ");
    hex(addr, 16);
    put("  pc ");
    hex(pc, 16);
    put("\nprobe stopped by an exception\n");
    arch::exit(false);
}
