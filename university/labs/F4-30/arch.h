// arch.h - F4-30: the arch layer for running DR402's arch-neutral code as a Linux user
// program under qemu-user, on many CPUs at once. It offers the names F3-18's panic.cc and
// F4-23's tests use (kName, kPageSize, cli, halt_forever, qemu_exit); the CPU-specific part
// is in ustart.cc, behind two system calls. A user program is not a kernel: it shows that
// the SAME source compiles and computes the same results on each CPU, not that a kernel boots.
#pragma once
#include <cstdint>

#ifndef ARCH_NAME
#error "build with -DARCH_NAME=\"...\""
#endif

namespace arch {

inline constexpr const char* kName = ARCH_NAME;
inline constexpr uint64_t kPageSize = 4096;   // the tests take it from here, never assume it
inline void cli() {}                           // a user program cannot mask interrupts
[[noreturn]] void halt_forever();
inline constexpr uint8_t kExitPass = 0x10;
inline constexpr uint8_t kExitFail = 0x11;
[[noreturn]] void qemu_exit(uint8_t code);     // here: the exit system call

} // namespace arch
