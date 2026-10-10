// memmap.cc - milestone A1: a UEFI application in C++ that prints the memory map.
// Built for the UEFI target with clang and lld-link (see run.sh); runs under OVMF in QEMU.
#include "console.hpp"
#include "efi.hpp"

namespace {

const char* type_name(uint32_t type)
{
    static const char* const names[efi::kMemoryTypeCount] = {
        "Reserved", "LoaderCode", "LoaderData", "BootServicesCode", "BootServicesData",
        "RuntimeServicesCode", "RuntimeServicesData", "Conventional", "Unusable",
        "ACPIReclaim", "ACPINVS", "MMIO", "MMIOPortSpace", "PalCode", "Persistent"};
    return type < efi::kMemoryTypeCount ? names[type] : "(unknown)";
}

void pad(Console& con, const char* text, int width)
{
    int n = 0;
    while (text[n] != '\0') {
        ++n;
    }
    con.print(text);
    for (; n < width; ++n) {
        con.print(" ");
    }
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    efi::BootServices* bs = st->boot_services;
    con.print("A1: hello from a UEFI application written in C++\n");

    // 1. Ask for the size first: with a zero-sized buffer the call fails with BUFFER_TOO_SMALL
    //    and reports the size it needs.
    uint64_t map_size = 0, map_key = 0, desc_size = 0;
    uint32_t desc_version = 0;
    efi::Status s = bs->get_memory_map(&map_size, nullptr, &map_key, &desc_size, &desc_version);
    if (s != efi::kBufferTooSmall) {
        con.print("unexpected status from GetMemoryMap: ");
        con.hex(s);
        con.print("\n");
        return s;
    }
    // 2. Allocating the buffer can itself add a descriptor, so ask for some spare room.
    map_size += 4 * desc_size;
    void* buffer = nullptr;
    s = bs->allocate_pool(efi::kLoaderData, map_size, &buffer);
    if (s != efi::kSuccess) {
        return s;
    }
    s = bs->get_memory_map(&map_size, static_cast<efi::MemoryDescriptor*>(buffer), &map_key,
                           &desc_size, &desc_version);
    if (s != efi::kSuccess) {
        con.print("GetMemoryMap failed: ");
        con.hex(s);
        con.print("\n");
        bs->free_pool(buffer);
        return s;
    }

    // 3. One line per descriptor. Step by desc_size, never by sizeof(MemoryDescriptor).
    const uint64_t count = map_size / desc_size;
    con.print("descriptor size ");
    con.dec(desc_size);
    con.print(" bytes, version ");
    con.dec(desc_version);
    con.print(", ");
    con.dec(count);
    con.print(" descriptors\n");
    con.print("  #  type                 physical start      pages     attribute\n");
    uint64_t pages_by_type[efi::kMemoryTypeCount] = {};
    auto* bytes = static_cast<uint8_t*>(buffer);
    for (uint64_t i = 0; i < count; ++i) {
        const auto* d = reinterpret_cast<const efi::MemoryDescriptor*>(bytes + i * desc_size);
        con.print(i < 10 ? "  " : (i < 100 ? " " : ""));
        con.dec(i);
        con.print("  ");
        pad(con, type_name(d->type), 21);
        con.hex(d->physical_start);
        con.print("  ");
        con.hex(d->number_of_pages, 8);
        con.print("  ");
        con.hex(d->attribute);
        con.print("\n");
        if (d->type < efi::kMemoryTypeCount) {
            pages_by_type[d->type] += d->number_of_pages;
        }
    }

    // 4. Totals per type, in 4 KiB pages and KiB.
    con.print("totals per type:\n");
    for (uint32_t t = 0; t < efi::kMemoryTypeCount; ++t) {
        if (pages_by_type[t] == 0) {
            continue;
        }
        con.print("  ");
        pad(con, type_name(t), 21);
        con.dec(pages_by_type[t]);
        con.print(" pages = ");
        con.dec(pages_by_type[t] * 4);
        con.print(" KiB\n");
    }
    // Memory the OS may use once boot services have exited (UEFI Specification, memory-type
    // usage after ExitBootServices; pending verification): loader, boot-services and free memory.
    const uint64_t usable = pages_by_type[efi::kLoaderCode] + pages_by_type[efi::kLoaderData] +
                            pages_by_type[efi::kBootServicesCode] +
                            pages_by_type[efi::kBootServicesData] +
                            pages_by_type[efi::kConventionalMemory];
    con.print("conventional KiB: ");
    con.dec(pages_by_type[efi::kConventionalMemory] * 4);
    con.print("\nusable after ExitBootServices KiB: ");
    con.dec(usable * 4);
    con.print("\nA1: done, returning to the firmware\n");
    bs->free_pool(buffer);
    return efi::kSuccess;
}
