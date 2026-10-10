// fuzz.h - the university's tiny coverage-guided mutation fuzzer (F11-09).
// It is the honest teaching version of a tool like libFuzzer or AFL++: a few
// hundred lines, no external library. Each lab that fuzzes a parser links this
// file together with a coverage runtime (cov.cc) and one target that provides
//     int fuzz_one(const unsigned char* data, unsigned long size);
// The target is compiled with -fsanitize-coverage=trace-pc so that every edge
// it runs sets a byte in a shared bitmap; fuzz.cc and cov.cc are compiled
// WITHOUT coverage, so the fuzzer does not drown in its own edges.
#ifndef UNIV_FUZZ_H
#define UNIV_FUZZ_H
#include <cstdint>

// Provided by the target (elf_target.cc, mav_target.cc, ...): parse one input.
// It must return normally for every input; a memory bug is detected by the
// sanitizer, which aborts the forked child, not by a return value.
int fuzz_one(const unsigned char* data, unsigned long size);

// Provided by cov.cc: the shared-memory edge bitmap (survives fork).
extern unsigned char* g_cov;      // COV_SIZE bytes, MAP_SHARED
extern const unsigned g_cov_size; // a power of two
void cov_init();                  // mmap the bitmap once, before any fork
#endif
