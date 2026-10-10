// check.cpp - F11-10: a tiny "checksec"-style reader. It opens its own two test
// binaries (built by run.sh: bin_weak and bin_hard) and reports, from the ELF
// program headers, whether the stack is executable (PT_GNU_STACK flags) and
// whether RELRO is present (PT_GNU_RELRO), and from the ELF header whether the
// file is position-independent (ET_DYN). This is the same information `readelf
// -l` prints, read by our own code so the lab does not depend on a tool's exact
// wording. Built and run by run_lab.sh; it must succeed.
#include <cstdio>
#include <cstdint>
#include <vector>

namespace {
constexpr uint32_t PT_GNU_STACK = 0x6474e551;
constexpr uint32_t PT_GNU_RELRO = 0x6474e552;
constexpr uint16_t ET_DYN = 3;

std::vector<unsigned char> slurp(const char* path)
{
    std::vector<unsigned char> b;
    FILE* f = std::fopen(path, "rb");
    if (!f) return b;
    unsigned char buf[8192];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) b.insert(b.end(), buf, buf + n);
    std::fclose(f);
    return b;
}
uint16_t r16(const std::vector<unsigned char>& b, size_t o) { return static_cast<uint16_t>(b[o] | (b[o + 1] << 8)); }
uint32_t r32(const std::vector<unsigned char>& b, size_t o)
{ return b[o] | (uint32_t(b[o + 1]) << 8) | (uint32_t(b[o + 2]) << 16) | (uint32_t(b[o + 3]) << 24); }
uint64_t r64(const std::vector<unsigned char>& b, size_t o)
{ return r32(b, o) | (uint64_t(r32(b, o + 4)) << 32); }

int report(const char* path)
{
    auto b = slurp(path);
    if (b.size() < 64 || b[0] != 0x7f) { std::printf("%s: not readable as ELF\n", path); return 1; }
    uint16_t e_type = r16(b, 16);
    uint64_t phoff = r64(b, 32);
    uint16_t phentsize = r16(b, 54);
    uint16_t phnum = r16(b, 56);

    bool have_stack = false, stack_exec = false, have_relro = false;
    for (uint16_t i = 0; i < phnum; ++i) {
        size_t ph = phoff + static_cast<size_t>(i) * phentsize;
        if (ph + 8 > b.size()) break;
        uint32_t type = r32(b, ph);
        uint32_t flags = r32(b, ph + 4);        // p_flags at offset 4 in ELF64 phdr
        if (type == PT_GNU_STACK) { have_stack = true; stack_exec = (flags & 1) != 0; }   // PF_X = 1
        if (type == PT_GNU_RELRO) have_relro = true;
    }
    std::printf("%-10s  PIE(ET_DYN)=%-3s  GNU_STACK=%-7s  exec-stack=%-3s  RELRO=%s\n",
                path, e_type == ET_DYN ? "yes" : "no",
                have_stack ? "present" : "absent", stack_exec ? "YES" : "no",
                have_relro ? "yes" : "no");
    return 0;
}
}  // namespace

int main(int argc, char** argv)
{
    int bad = 0;
    for (int i = 1; i < argc; ++i) bad += report(argv[i]);
    if (argc < 2) { std::printf("usage: check <elf>...\n"); return 2; }
    return bad;
}
