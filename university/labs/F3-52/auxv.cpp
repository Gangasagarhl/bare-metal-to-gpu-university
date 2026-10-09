// auxv.cpp - F3-52: what the kernel told this process at exec time, through the auxiliary
// vector, compared with what the dynamic linker reports about the loaded objects. The values
// change from run to run (address-space layout randomisation); the "matches" lines must not.
#include <cstdio>
#include <cstring>
#include <link.h>
#include <sys/auxv.h>

extern "C" char _start[];                     // the program's entry point, from the start file

namespace {

struct Found
{
    const ElfW(Phdr)* main_phdr = nullptr;     // program headers of the first object (the program)
    ElfW(Addr) interp_base = 0;                // load address of the dynamic linker
    int objects = 0;
};

int visit(dl_phdr_info* info, size_t, void* data)
{
    Found* f = static_cast<Found*>(data);
    if (f->objects == 0) {
        f->main_phdr = info->dlpi_phdr;
    }
    if (std::strstr(info->dlpi_name, "ld-linux") != nullptr) {
        f->interp_base = info->dlpi_addr;
    }
    ++f->objects;
    return 0;
}

const char* yes(bool b) { return b ? "yes" : "NO"; }

} // namespace

int main()
{
    unsigned long phdr = getauxval(AT_PHDR);
    unsigned long phnum = getauxval(AT_PHNUM);
    unsigned long entry = getauxval(AT_ENTRY);
    unsigned long base = getauxval(AT_BASE);
    unsigned long pagesz = getauxval(AT_PAGESZ);
    unsigned long random = getauxval(AT_RANDOM);
    Found f;
    dl_iterate_phdr(visit, &f);

    std::printf("AT_PHDR   = 0x%lx  (program headers of the program, %lu entries)\n", phdr, phnum);
    std::printf("AT_ENTRY  = 0x%lx  (where the program starts after the dynamic linker)\n", entry);
    std::printf("AT_BASE   = 0x%lx  (where the kernel loaded the dynamic linker)\n", base);
    std::printf("AT_PAGESZ = %lu\n", pagesz);
    std::printf("AT_RANDOM = %s  (address of 16 random bytes from the kernel)\n",
                random != 0 ? "present" : "missing");
    std::printf("objects reported by dl_iterate_phdr: %d\n", f.objects);
    std::printf("AT_PHDR matches the first object's program headers: %s\n",
                yes(phdr == reinterpret_cast<unsigned long>(f.main_phdr)));
    std::printf("AT_ENTRY matches the address of _start: %s\n",
                yes(entry == reinterpret_cast<unsigned long>(_start)));
    std::printf("AT_BASE matches the dynamic linker's load address: %s\n",
                yes(base == f.interp_base));
    return 0;
}
