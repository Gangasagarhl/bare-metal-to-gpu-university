// loader.cc - milestone A3: a UEFI loader in C++ that loads an ELF64 kernel from the ESP,
// builds page tables, gathers boot information, exits boot services and jumps to the kernel.
// Build: clang for the UEFI target (see run.sh). Defining PLANT_STALE_KEY builds the broken
// variant used as forensic evidence: it allocates memory between GetMemoryMap and
// ExitBootServices and does not retry.
#include "bootinfo.hpp"
#include "console.hpp"
#include "efi.hpp"
#include "elf64.hpp"

// A freestanding program must supply these two itself: the compiler emits calls to them for
// zero-initialised structures and structure copies. Volatile keeps the compiler from turning
// the loops back into calls to themselves.
extern "C" void* memset(void* dest, int value, size_t n)
{
    auto* d = static_cast<volatile uint8_t*>(dest);
    for (size_t i = 0; i < n; ++i) {
        d[i] = static_cast<uint8_t>(value);
    }
    return dest;
}

extern "C" void* memcpy(void* dest, const void* src, size_t n)
{
    auto* d = static_cast<volatile uint8_t*>(dest);
    const auto* s = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dest;
}

namespace {

constexpr uint64_t kPage = 4096;
constexpr uint64_t kPresent = 1, kWritable = 2, kLarge = 1ull << 7, kNoExecute = 1ull << 63;
constexpr uint64_t kAddrMask = 0x000ffffffffff000ull;

efi::BootServices* bs;
Console* con;

uint64_t rdtsc()
{
    uint32_t lo = 0, hi = 0;
    __asm__ volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return static_cast<uint64_t>(hi) << 32 | lo;
}

void serial(const char* s)  // after ExitBootServices the console protocol is gone: use COM1
{
    for (; *s != '\0'; ++s) {
        uint8_t lsr = 0;
        do {
            __asm__ volatile("inb %1, %0" : "=a"(lsr) : "Nd"(static_cast<uint16_t>(0x3fd)));
        } while ((lsr & 0x20) == 0);
        __asm__ volatile("outb %0, %1" : : "a"(static_cast<uint8_t>(*s)), "Nd"(static_cast<uint16_t>(0x3f8)));
    }
}

uint64_t pages(uint64_t count, efi::MemoryType type = efi::kLoaderData)  // zeroed pages or 0
{
    uint64_t address = 0;
    if (bs->allocate_pages(efi::kAllocateAnyPages, type, count, &address) != efi::kSuccess) {
        return 0;
    }
    memset(reinterpret_cast<void*>(address), 0, count * kPage);
    return address;
}

efi::Status fail(const char* what, efi::Status s)
{
    con->print("loader: ");
    con->print(what);
    con->print(" (status ");
    con->hex(s);
    con->print("), returning to the firmware\n");
    return s;
}

// Reads a whole file from the root of the volume the loader came from. Returns its size, 0 on error.
uint64_t read_file(efi::File* root, const efi::Char16* name, uint8_t** out)
{
    efi::File* file = nullptr;
    if (root->open(root, &file, name, efi::kFileModeRead, 0) != efi::kSuccess) {
        return 0;
    }
    alignas(8) uint8_t info_buf[256];
    uint64_t info_size = sizeof info_buf;
    if (file->get_info(file, &efi::kFileInfoGuid, &info_size, info_buf) != efi::kSuccess) {
        file->close(file);
        return 0;
    }
    uint64_t size = reinterpret_cast<efi::FileInfo*>(info_buf)->file_size;
    void* buffer = nullptr;
    if (size == 0 || bs->allocate_pool(efi::kLoaderData, size, &buffer) != efi::kSuccess ||
        file->read(file, &size, buffer) != efi::kSuccess) {
        file->close(file);
        return 0;
    }
    file->close(file);
    *out = static_cast<uint8_t*>(buffer);
    return size;
}

// Returns the page-table entry slot for `virt` at the 4 KiB level, creating tables on the way.
uint64_t* pte_slot(uint64_t pml4, uint64_t virt)
{
    uint64_t table = pml4;
    for (int shift = 39; shift > 12; shift -= 9) {
        auto* entry = reinterpret_cast<uint64_t*>(table) + ((virt >> shift) & 0x1ff);
        if ((*entry & kPresent) == 0) {
            const uint64_t next = pages(1);
            if (next == 0) {
                return nullptr;
            }
            *entry = next | kPresent | kWritable;  // permissions are decided at the last level
        }
        table = *entry & kAddrMask;
    }
    return reinterpret_cast<uint64_t*>(table) + ((virt >> 12) & 0x1ff);
}

const void* find_table(efi::SystemTable* st, const efi::Guid& guid)
{
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        if (efi::same(st->configuration_table[i].vendor_guid, guid)) {
            return st->configuration_table[i].vendor_table;
        }
    }
    return nullptr;
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle image, efi::SystemTable* st)
{
    const uint64_t tsc_entry = rdtsc();
    Console console(st->con_out);
    con = &console;
    bs = st->boot_services;
    bs->set_watchdog_timer(0, 0, 0, nullptr);  // firmware watchdog off while we work
    con->print("loader: OS301 loader started\n");

    // 1. the volume we were loaded from, and the kernel file on it
    efi::LoadedImage* li = nullptr;
    efi::SimpleFileSystem* fs = nullptr;
    efi::File* root = nullptr;
    efi::Status s = bs->handle_protocol(image, &efi::kLoadedImageProtocolGuid, reinterpret_cast<void**>(&li));
    if (s == efi::kSuccess) {
        s = bs->handle_protocol(li->device_handle, &efi::kSimpleFileSystemProtocolGuid, reinterpret_cast<void**>(&fs));
    }
    if (s == efi::kSuccess) {
        s = fs->open_volume(fs, &root);
    }
    if (s != efi::kSuccess) {
        return fail("cannot open the boot volume", s);
    }
    uint8_t* file = nullptr;
    const uint64_t file_size = read_file(root, u"\\kernel.elf", &file);
    if (file_size == 0) {
        return fail("cannot read \\kernel.elf", efi::kNotFound);
    }
    const uint64_t tsc_read = rdtsc();
    con->print("loader: read \\kernel.elf, ");
    con->dec(file_size);
    con->print(" bytes\n");

    // 2. check the ELF image before touching memory for it
    elf::Image img;
    if (const char* why = elf::check(file, file_size, &img)) {
        con->print("loader: kernel.elf rejected: ");
        con->print(why);
        con->print("\n");
        return fail("not jumping to a bad kernel", efi::kLoadError);
    }

    // 3. one physically contiguous block for the whole kernel span; copy and zero-fill segments
    const uint64_t span = img.highest - img.lowest;
    const uint64_t kphys = pages(span / kPage);
    if (kphys == 0) {
        return fail("no memory for the kernel", efi::kBufferTooSmall);
    }
    for (int i = 0; i < img.count; ++i) {
        const elf::Segment& seg = img.seg[i];
        memcpy(reinterpret_cast<void*>(kphys + (seg.vaddr - img.lowest)), file + seg.offset, seg.filesz);
        // bytes from filesz to memsz stay zero: pages() returned zeroed memory
    }

    // 4. boot information: allocated now, filled now, memory map added at the very end
    const uint64_t bi_phys = pages(1);
    auto* bi = reinterpret_cast<os301::BootInfo*>(bi_phys);
    bi->magic = os301::kBootMagic;
    bi->version = os301::kBootInfoVersion;
    bi->size = sizeof(os301::BootInfo);
    bi->direct_map_base = os301::kDirectMapBase;
    bi->kernel_physical = kphys;
    bi->kernel_virtual = img.lowest;
    bi->kernel_size = span;
    bi->rsdp_physical = reinterpret_cast<uint64_t>(find_table(st, efi::kAcpi20TableGuid));
    bi->smbios3_physical = reinterpret_cast<uint64_t>(find_table(st, efi::kSmbios3TableGuid));
    efi::GraphicsOutput* gop = nullptr;
    if (bs->locate_protocol(&efi::kGraphicsOutputProtocolGuid, nullptr, reinterpret_cast<void**>(&gop)) == efi::kSuccess) {
        bi->fb_physical = gop->mode->frame_buffer_base;
        bi->fb_size = gop->mode->frame_buffer_size;
        bi->fb_width = gop->mode->info->horizontal_resolution;
        bi->fb_height = gop->mode->info->vertical_resolution;
        bi->fb_pitch = gop->mode->info->pixels_per_scan_line * 4;
        bi->fb_format = gop->mode->info->pixel_format;
    }
    uint8_t* cmdline = nullptr;
    const uint64_t cmdline_size = read_file(root, u"\\cmdline.txt", &cmdline);
    for (uint64_t i = 0; i < cmdline_size && i < sizeof bi->command_line - 1 && cmdline[i] != '\n'; ++i) {
        bi->command_line[i] = static_cast<char>(cmdline[i]);
    }
    bi->tsc_loader_entry = tsc_entry;
    bi->tsc_kernel_file_read = tsc_read;

    // 5. page tables: kernel segments with their own permissions; identity map and direct map of
    //    all memory below the highest address in the memory map, with 2 MiB pages
    const uint64_t pml4 = pages(1);
    for (int i = 0; i < img.count; ++i) {
        const elf::Segment& seg = img.seg[i];
        for (uint64_t off = 0; off < seg.memsz; off += kPage) {
            uint64_t* pte = pte_slot(pml4, seg.vaddr + off);
            if (pte == nullptr) {
                return fail("no memory for page tables", efi::kBufferTooSmall);
            }
            *pte = (kphys + (seg.vaddr - img.lowest) + off) | kPresent |
                   ((seg.flags & elf::kFlagW) ? kWritable : 0) | ((seg.flags & elf::kFlagX) ? 0 : kNoExecute);
        }
    }
    uint64_t map_size = 0, map_key = 0, desc_size = 0;
    uint32_t desc_version = 0;
    bs->get_memory_map(&map_size, nullptr, &map_key, &desc_size, &desc_version);
    const uint64_t map_capacity = map_size + 16 * desc_size;   // room for the map to grow
    const uint64_t map_phys = pages((map_capacity + kPage - 1) / kPage);
    const uint64_t ranges_phys = pages((map_capacity / desc_size * sizeof(os301::MemoryRange) + kPage - 1) / kPage);
    map_size = map_capacity;
    s = bs->get_memory_map(&map_size, reinterpret_cast<efi::MemoryDescriptor*>(map_phys), &map_key, &desc_size, &desc_version);
    if (s != efi::kSuccess || map_phys == 0 || ranges_phys == 0) {
        return fail("GetMemoryMap failed", s);
    }
    uint64_t top = 4ull << 30;            // at least 4 GiB: the framebuffer and devices live below it
    for (uint64_t off = 0; off < map_size; off += desc_size) {
        const auto* d = reinterpret_cast<const efi::MemoryDescriptor*>(map_phys + off);
        const uint64_t end = d->physical_start + d->number_of_pages * kPage;
        if (d->type != efi::kMemoryMappedIO && d->type != efi::kMemoryMappedIOPortSpace && d->type != efi::kReservedMemoryType) {
            top = end > top ? end : top;  // RAM of every kind; device windows above 4 GiB are left out
        }
    }
    const uint64_t gib = (top + (1ull << 30) - 1) >> 30;
    if (gib > 512) {
        return fail("more than 512 GiB of RAM: this loader's direct map is too small", efi::kUnsupported);
    }
    const uint64_t pdpt = pages(1);
    const uint64_t pds = pages(gib);
    if (pdpt == 0 || pds == 0) {
        return fail("no memory for the direct map", efi::kBufferTooSmall);
    }
    for (uint64_t g = 0; g < gib; ++g) {
        reinterpret_cast<uint64_t*>(pdpt)[g] = (pds + g * kPage) | kPresent | kWritable;
        for (uint64_t m = 0; m < 512; ++m) {
            reinterpret_cast<uint64_t*>(pds + g * kPage)[m] = ((g << 30) + (m << 21)) | kPresent | kWritable | kLarge;
        }
    }
    reinterpret_cast<uint64_t*>(pml4)[0] = pdpt | kPresent | kWritable;    // identity map (for the switch)
    reinterpret_cast<uint64_t*>(pml4)[256] = pdpt | kPresent | kWritable;  // direct map at kDirectMapBase
    const uint64_t stack = pages(16);                                       // 64 KiB kernel stack
    if (stack == 0) {
        return fail("no memory for the kernel stack", efi::kBufferTooSmall);
    }
    con->print("loader: kernel at physical ");
    con->hex(kphys);
    con->print(", entry ");
    con->hex(img.entry);
    con->print(", page tables at ");
    con->hex(pml4);
    con->print(", direct map of ");
    con->dec(gib);
    con->print(" GiB\nloader: exiting boot services\n");

    // 6. the handoff: final memory map, ExitBootServices with its key, retry if the key is stale
    bi->tsc_before_exit = rdtsc();
    uint32_t attempts = 0;
    for (;;) {
        map_size = map_capacity;
        s = bs->get_memory_map(&map_size, reinterpret_cast<efi::MemoryDescriptor*>(map_phys), &map_key, &desc_size,
                               &desc_version);
        if (s != efi::kSuccess) {
            return fail("GetMemoryMap failed", s);
        }
#if PLANT_STALE_KEY == 1
        void* log_line = nullptr;                    // planted bug, variant 1: a "harmless" small allocation
        bs->allocate_pool(efi::kLoaderData, 64, &log_line);
#elif PLANT_STALE_KEY == 2
        uint64_t scratch = 0;                        // planted bug, variant 2: one more page
        bs->allocate_pages(efi::kAllocateAnyPages, efi::kLoaderData, 1, &scratch);
#endif
        ++attempts;
        s = bs->exit_boot_services(image, map_key);
        if (s == efi::kSuccess) {
            break;
        }
#ifdef PLANT_STALE_KEY
        con->print("loader: ExitBootServices failed (status ");
        con->hex(s);
        con->print(") with map key ");
        con->hex(map_key);
        con->print("\n");
        for (;;) {                                   // and no retry: the screen keeps the logo
            __asm__ volatile("pause");
        }
#endif
        if (attempts == 4) {
            return fail("ExitBootServices keeps failing", s);  // boot services are still usable here
        }
    }
    // From here on: no boot service, no console protocol, no allocation.
    bi->tsc_after_exit = rdtsc();
    bi->exit_attempts = attempts;
    auto* ranges = reinterpret_cast<os301::MemoryRange*>(ranges_phys);
    bi->memory_map_count = map_size / desc_size;
    for (uint64_t i = 0; i < bi->memory_map_count; ++i) {
        const auto* d = reinterpret_cast<const efi::MemoryDescriptor*>(map_phys + i * desc_size);
        ranges[i] = {d->type, 0, d->physical_start, d->number_of_pages, d->attribute};
    }
    bi->memory_map = os301::kDirectMapBase + ranges_phys;
    serial(attempts == 1 ? "loader: ExitBootServices succeeded on the first attempt\r\n"
                         : "loader: ExitBootServices succeeded after a retry\r\n");
    serial("loader: jumping to the kernel\r\n");

    // 7. switch to our page tables and jump: interrupts off, no-execute enabled, new stack
    uint32_t lo = 0, hi = 0;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0xc0000080));  // EFER
    lo |= 1u << 11;                                                     // NXE: honour bit 63
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(0xc0000080));
    const uint64_t stack_top = os301::kDirectMapBase + stack + 16 * kPage - 8;  // as if called
    const uint64_t arg = os301::kDirectMapBase + bi_phys;
    __asm__ volatile(
        "cli\n\t"
        "mov %0, %%cr3\n\t"
        "mov %1, %%rsp\n\t"
        "xor %%ebp, %%ebp\n\t"
        "jmp *%2\n\t"
        :
        : "r"(pml4), "r"(stack_top), "r"(img.entry), "D"(arg)
        : "memory");
    __builtin_unreachable();
}
