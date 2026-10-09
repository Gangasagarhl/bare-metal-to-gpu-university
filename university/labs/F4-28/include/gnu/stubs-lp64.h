/* stubs-lp64.h - F4-28: intentionally empty.
 * The cross compiler's C++ headers (<cstdint>, <atomic>) include glibc's <gnu/stubs.h>, which
 * includes a file per ABI. The Ubuntu riscv64 glibc ships only the hard-float one
 * (stubs-lp64d.h), but this kernel is built for the soft-float ABI lp64 so that the compiler
 * never touches floating-point registers the trap handler does not save. glibc's stubs file
 * only lists unimplemented C-library functions, which a freestanding kernel never calls. */
