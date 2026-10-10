// elf_tests.cpp - unit tests for the FIXED ELF reader. Built and run by
// run_lab.sh with -fsanitize=address,undefined: if the fixed reader over-reads
// on any of these crafted malformed inputs, AddressSanitizer aborts and the
// test fails. The fixed reader must simply return on every input.
#define FIXED
#include "elf_target.cc"      // brings in fuzz_one (the fixed version)

#include <cstdio>
#include <vector>

namespace {
int failures = 0;

void feed(const char* what, std::vector<unsigned char> in)
{
    // Any memory bug here is caught by the sanitizer (which aborts the process),
    // so reaching the next line already means "did not over-read".
    fuzz_one(in.data(), in.size());
    std::printf("ok: %s (%zu bytes parsed without over-read)\n", what, in.size());
}

std::vector<unsigned char> min_elf()
{
    std::vector<unsigned char> b(64, 0);
    b[0] = 0x7f; b[1] = 'E'; b[2] = 'L'; b[3] = 'F';
    b[4] = 2; b[5] = 1;                       // ELFCLASS64, little-endian
    b[58] = 64;                               // e_shentsize = 64
    b[60] = 1;                                // e_shnum = 1
    return b;
}
}  // namespace

int main()
{
    feed("too short", std::vector<unsigned char>(10, 0));
    feed("not ELF", std::vector<unsigned char>(64, 0x55));

    std::vector<unsigned char> t = min_elf();
    feed("header only, shoff 0", t);

    // sh_name enormous, shstrtab offset points nowhere: the fix must skip it.
    t = min_elf();
    t[62] = 0;                                 // e_shstrndx = 0
    // give section 0 a huge sh_name at offset 0 of its header (e_shoff = 0 here)
    t[0 + 0] = 0xff; t[0 + 1] = 0xff; t[0 + 2] = 0xff; t[0 + 3] = 0x7f;
    // but offset 0 is the magic; set a real shoff instead:
    t = min_elf();
    t[40] = 64;                                // e_shoff = 64 (just past the header)
    t.resize(64 + 64, 0);                      // one 64-byte section header, all zero
    t[64 + 0] = 0xff; t[64 + 1] = 0xff; t[64 + 2] = 0xff; t[64 + 3] = 0x0f;  // sh_name huge
    feed("huge sh_name", t);

    // shstrtab offset far past the end.
    t = min_elf();
    t[40] = 64; t.resize(128, 0);
    t[64 + 24] = 0xff; t[64 + 25] = 0xff;      // sh_offset huge (bytes of str header)
    feed("shstrtab offset past end", t);

    std::printf(failures ? "SOME TESTS FAILED\n" : "ALL TESTS PASSED\n");
    return failures;
}
