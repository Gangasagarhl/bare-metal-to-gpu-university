// efi_a64.h - F4-33: the few UEFI declarations the EBBR probe needs, for AArch64.
//
// Written for this course (no EDK II headers in the build). Structure layouts follow the UEFI
// Specification's chapters "EFI System Table" and "Services - Boot Services" (title only - not
// opened during this build). Every unused function slot is kept as void* so that later slots
// stay at the right offset. What the lab shows is only that the firmware in QEMU accepted code
// built on them; check every layout against the specification before reusing this file.
// On AArch64, UEFI uses the standard Arm procedure-call convention, so no calling-convention
// attribute is needed (unlike x86-64, where UEFI uses the Microsoft convention).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace efi {

using Status = uint64_t;
using Handle = void*;
constexpr Status kErrorBit = 0x8000000000000000ull;
constexpr Status kSuccess = 0;
constexpr Status kBufferTooSmall = kErrorBit | 5;

struct Guid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};

struct TableHeader {
    uint64_t signature;
    uint32_t revision;        // major in the upper 16 bits, minor in the lower 16 bits
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
};

struct TextOut {
    void* reset;
    Status (*output_string)(TextOut* self, const char16_t* text);
};

struct MemoryDescriptor {
    uint32_t type;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;  // 4 KiB pages
    uint64_t attribute;
};
constexpr uint32_t kConventionalMemory = 7;   // position of EfiConventionalMemory in EFI_MEMORY_TYPE
constexpr uint32_t kLoaderData = 2;

struct BootServices {
    TableHeader hdr;
    void* raise_tpl;
    void* restore_tpl;
    void* allocate_pages;
    void* free_pages;
    Status (*get_memory_map)(uint64_t* size, MemoryDescriptor* map, uint64_t* key, uint64_t* desc_size,
                             uint32_t* desc_version);
    Status (*allocate_pool)(uint32_t type, uint64_t size, void** buffer);
    Status (*free_pool)(void* buffer);
};

struct ConfigurationTable {
    Guid vendor_guid;
    void* vendor_table;
};

struct SystemTable {
    TableHeader hdr;
    char16_t* firmware_vendor;
    uint32_t firmware_revision;
    Handle console_in_handle;
    void* con_in;
    Handle console_out_handle;
    TextOut* con_out;
    Handle standard_error_handle;
    TextOut* std_err;
    void* runtime_services;
    BootServices* boot_services;
    uint64_t number_of_table_entries;
    ConfigurationTable* configuration_table;
};

}  // namespace efi
