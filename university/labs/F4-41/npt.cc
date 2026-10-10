// npt.cc - nested page tables, the ELF loader and the guest's entry state.
// Nested entries use the long-mode page-table format; nested walks are treated as user
// accesses, so every level sets U/S (AMD64 APM Vol. 2, "Nested Paging": from memory, not
// opened in this build; npt_probe and the guest runs prove it on QEMU 8.2.2 only).
#include "npt.h"
#include "kio.h"

namespace {
alignas(2 << 20) uint8_t g_ram[GUEST_RAM_SIZE];     // the guest's "physical" memory
alignas(4096) uint64_t g_npml4[512];
alignas(4096) uint64_t g_npdpt[512];
alignas(4096) uint64_t g_npd[512];
constexpr uint64_t P = 1, RW = 2, US = 4, PS = 0x80;
constexpr uint64_t GPA_MBI = 0x9000, GPA_CMDLINE = 0x9100;    // Multiboot information
constexpr uint64_t GPA_PT = 0x10000;                          // guest page tables: 6 pages

struct Elf64Ehdr {
    uint8_t ident[16];
    uint16_t type, machine;
    uint32_t version;
    uint64_t entry, phoff, shoff;
    uint32_t flags;
    uint16_t ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};
struct Elf64Phdr { uint32_t type, flags; uint64_t offset, vaddr, paddr, filesz, memsz, align; };
struct Elf64Shdr { uint32_t name, type; uint64_t flags, addr, offset, size;
                   uint32_t link, info; uint64_t addralign, entsize; };
struct Elf64Sym { uint32_t name; uint8_t info, other; uint16_t shndx; uint64_t value, size; };

uint64_t find_symbol(const uint8_t* elf, const Elf64Ehdr* eh, const char* want)
{
    for (uint16_t i = 0; i < eh->shnum; ++i) {
        const auto* sh = reinterpret_cast<const Elf64Shdr*>(elf + eh->shoff + i * eh->shentsize);
        if (sh->type != 2) {                                      // SHT_SYMTAB
            continue;
        }
        const auto* strtab = reinterpret_cast<const Elf64Shdr*>(elf + eh->shoff +
                                                                sh->link * eh->shentsize);
        const char* names = reinterpret_cast<const char*>(elf + strtab->offset);
        for (uint64_t k = 0; k < sh->size / sizeof(Elf64Sym); ++k) {
            const auto* sym = reinterpret_cast<const Elf64Sym*>(elf + sh->offset) + k;
            const char* n = names + sym->name;
            if (memcmp(n, want, kstrlen(want) + 1) == 0) {
                return sym->value;
            }
        }
    }
    return 0;
}
}

uint8_t* guest_ram() { return g_ram; }

uint64_t npt_build()
{
    // guest-physical [0, 32 MiB) -> host-physical g_ram, with 2 MiB pages.
    // Everything else is left not-present: a guest access there is a nested page fault.
    uint64_t host = reinterpret_cast<uint64_t>(g_ram);   // host identity map: VA = PA
    for (uint64_t i = 0; i < GUEST_RAM_SIZE >> 21; ++i) {
        g_npd[i] = (host + (i << 21)) | P | RW | US | PS;
    }
    g_npdpt[0] = reinterpret_cast<uint64_t>(g_npd) | P | RW | US;
    g_npml4[0] = reinterpret_cast<uint64_t>(g_npdpt) | P | RW | US;
    return reinterpret_cast<uint64_t>(g_npml4);
}

bool load_guest_elf(const uint8_t* elf, uint64_t size, GuestImage& img)
{
    const auto* eh = reinterpret_cast<const Elf64Ehdr*>(elf);
    if (size < sizeof *eh || memcmp(eh->ident, "\x7f" "ELF", 4) != 0 || eh->ident[4] != 2) {
        kprintf("hv: guest image is not an ELF64 file\n");
        return false;
    }
    for (uint16_t i = 0; i < eh->phnum; ++i) {
        const auto* ph = reinterpret_cast<const Elf64Phdr*>(elf + eh->phoff + i * eh->phentsize);
        if (ph->type != 1) {                                       // PT_LOAD only
            continue;
        }
        if (ph->paddr + ph->memsz > GUEST_RAM_SIZE || ph->offset + ph->filesz > size) {
            kprintf("hv: segment at %lx does not fit in guest RAM\n", ph->paddr);
            return false;
        }
        memcpy(g_ram + ph->paddr, elf + ph->offset, ph->filesz);
        memset(g_ram + ph->paddr + ph->filesz, 0, ph->memsz - ph->filesz);   // .bss
        kprintf("hv: loaded segment gpa %lx, %lu bytes (%lu from the file)\n", ph->paddr,
                ph->memsz, ph->filesz);
    }
    img.entry32 = eh->entry;
    img.long_mode = find_symbol(elf, eh, "long_mode");
    img.boot_magic = find_symbol(elf, eh, "boot_magic");
    img.boot_info = find_symbol(elf, eh, "boot_info");
    img.gdt64_ptr = find_symbol(elf, eh, "gdt64_ptr");
    kprintf("hv: symbols: long_mode %lx boot_magic %lx boot_info %lx gdt64_ptr %lx\n",
            img.long_mode, img.boot_magic, img.boot_info, img.gdt64_ptr);
    return img.long_mode && img.boot_magic && img.boot_info && img.gdt64_ptr;
}

void vcpu_init_guest_kernel(Vcpu& v, Vmcb* vmcb, const GuestImage& img, const char* cmdline,
                            uint64_t ncr3)
{
    // 1. Multiboot information with the command line, as QEMU's loader would give it.
    auto* mbi = reinterpret_cast<uint32_t*>(g_ram + GPA_MBI);
    memset(mbi, 0, 128);
    mbi[0] = 1u << 2;                                   // flags: cmdline valid
    mbi[4] = GPA_CMDLINE;
    memcpy(g_ram + GPA_CMDLINE, cmdline, kstrlen(cmdline) + 1);
    *reinterpret_cast<uint32_t*>(g_ram + img.boot_magic) = 0x2BADB002;
    *reinterpret_cast<uint32_t*>(g_ram + img.boot_info) = GPA_MBI;
    // 2. Guest page tables in guest memory: identity map of 4 GiB with 2 MiB pages
    //    (the same map boot.S builds). Entries hold guest-physical addresses.
    auto* pt = reinterpret_cast<uint64_t*>(g_ram + GPA_PT);
    memset(pt, 0, 6 * 4096);
    pt[0] = (GPA_PT + 0x1000) | P | RW;                 // PML4[0] -> PDPT
    for (uint64_t j = 0; j < 4; ++j) {
        pt[512 + j] = (GPA_PT + 0x2000 + j * 0x1000) | P | RW;   // PDPT[j] -> PD j
        for (uint64_t i = 0; i < 512; ++i) {
            pt[1024 + j * 512 + i] = ((j << 30) + (i << 21)) | P | RW | PS;
        }
    }
    // 3. The VMCB: intercepts as in F4-40, then nested paging and a 64-bit entry state.
    vcpu_init_long_mode(v, vmcb, img.long_mode, 0);
    Vmcb& c = *vmcb;
    c.u64(vmcb::NP_ENABLE) = 1;
    c.u64(vmcb::N_CR3) = ncr3;
    uint16_t gdt_limit = *reinterpret_cast<uint16_t*>(g_ram + img.gdt64_ptr);
    uint64_t gdt_base = *reinterpret_cast<uint64_t*>(g_ram + img.gdt64_ptr + 2);
    c.seg(vmcb::GDTR, 0, 0, gdt_limit, gdt_base);       // the kernel's own GDT (gpa)
    c.u64(vmcb::CR0) = 0x80000011;                      // PG | ET | PE
    c.u64(vmcb::CR4) = 0x20;                            // PAE
    c.u64(vmcb::CR3) = GPA_PT;                          // a guest-physical address
    c.u64(vmcb::EFER) = 0x1500;                         // SVME | LMA | LME
    c.u64(vmcb::G_PAT) = 0x0007040600070406ull;         // the power-on PAT value
#ifdef F441_ENTRY32
    // The forensic build: enter at _start like QEMU's Multiboot loader does, in 32-bit
    // protected mode with paging off (EAX = magic, EBX = information address).
    c.seg(vmcb::CS, 0x08, 0x0C9B, 0xFFFFFFFF, 0);
    c.seg(vmcb::DS, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    c.seg(vmcb::ES, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    c.seg(vmcb::SS, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    c.u64(vmcb::CR0) = 0x11;
    c.u64(vmcb::CR3) = 0;
    c.u64(vmcb::CR4) = 0;
    c.u64(vmcb::EFER) = 1ull << 12;
    c.u64(vmcb::RIP) = img.entry32;
    c.u64(vmcb::RAX) = 0x2BADB002;
    v.regs.rbx = GPA_MBI;
#endif
}
