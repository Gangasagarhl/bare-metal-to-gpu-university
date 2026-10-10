// efi.hpp - the small subset of UEFI declarations used by the OS301 labs (F3-10 to F3-16), copied
// into F11-02 with SetVariable, more status codes and the db/dbx and signature-type GUIDs added.
//
// Written for this course instead of using EDK II or gnu-efi headers (route (b) of milestone A1:
// clang targeting x86_64-unknown-windows, lld-link producing PE32+ directly).
// Names follow the UEFI Specification's names in C++ style. The layouts were written for this
// build and NOT checked against the specification text (no document could be opened); what the
// lab runs show is that OVMF accepted and ran code built on them. Check every structure against
// the UEFI Specification sections named in the comments before reusing this file.
//
// Rule kept everywhere: a table of function pointers keeps every slot in order, even slots this
// course never calls (they are declared as void* so that the offsets of the later slots are right).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace efi {

using Status = uint64_t;   // UINTN status code; the top bit marks an error
using Handle = void*;
using Char16 = char16_t;   // UCS-2 text, as the console protocol expects

constexpr Status kErrorBit = 0x8000000000000000ull;
constexpr Status kSuccess = 0;
constexpr Status kLoadError = kErrorBit | 1;
constexpr Status kInvalidParameter = kErrorBit | 2;
constexpr Status kUnsupported = kErrorBit | 3;
constexpr Status kBufferTooSmall = kErrorBit | 5;
constexpr Status kWriteProtected = kErrorBit | 8;
constexpr Status kOutOfResources = kErrorBit | 9;
constexpr Status kNotFound = kErrorBit | 14;
constexpr Status kAccessDenied = kErrorBit | 15;
constexpr Status kSecurityViolation = kErrorBit | 26;

struct Guid {               // UEFI Specification: EFI_GUID
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
};

inline bool same(const Guid& a, const Guid& b)
{
    if (a.data1 != b.data1 || a.data2 != b.data2 || a.data3 != b.data3) {
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        if (a.data4[i] != b.data4[i]) {
            return false;
        }
    }
    return true;
}

struct TableHeader {        // UEFI Specification: EFI_TABLE_HEADER
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
};

struct SimpleTextOutput {   // UEFI Specification: Simple Text Output Protocol
    void* reset;
    Status (*output_string)(SimpleTextOutput* self, const Char16* text);
    // later slots (TestString, QueryMode, SetMode, ...) are not used
};

// UEFI Specification: EFI_MEMORY_TYPE (the numbers are the order of the enumeration)
enum MemoryType : uint32_t {
    kReservedMemoryType, kLoaderCode, kLoaderData, kBootServicesCode, kBootServicesData,
    kRuntimeServicesCode, kRuntimeServicesData, kConventionalMemory, kUnusableMemory,
    kACPIReclaimMemory, kACPIMemoryNVS, kMemoryMappedIO, kMemoryMappedIOPortSpace, kPalCode,
    kPersistentMemory, kMemoryTypeCount
};

struct MemoryDescriptor {   // UEFI Specification: EFI_MEMORY_DESCRIPTOR
    uint32_t type;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages; // 4 KiB pages
    uint64_t attribute;
};

enum AllocateType : uint32_t { kAllocateAnyPages, kAllocateMaxAddress, kAllocateAddress };

struct BootServices {       // UEFI Specification: EFI_BOOT_SERVICES (slots in table order)
    TableHeader hdr;
    void* raise_tpl;
    void* restore_tpl;
    Status (*allocate_pages)(AllocateType type, MemoryType mtype, uint64_t pages, uint64_t* address);
    Status (*free_pages)(uint64_t address, uint64_t pages);
    Status (*get_memory_map)(uint64_t* map_size, MemoryDescriptor* map, uint64_t* map_key,
                             uint64_t* descriptor_size, uint32_t* descriptor_version);
    Status (*allocate_pool)(MemoryType type, uint64_t size, void** buffer);
    Status (*free_pool)(void* buffer);
    void* create_event;
    void* set_timer;
    void* wait_for_event;
    void* signal_event;
    void* close_event;
    void* check_event;
    void* install_protocol_interface;
    void* reinstall_protocol_interface;
    void* uninstall_protocol_interface;
    Status (*handle_protocol)(Handle handle, const Guid* protocol, void** interface);
    void* reserved;
    void* register_protocol_notify;
    void* locate_handle;
    void* locate_device_path;
    void* install_configuration_table;
    void* load_image;
    void* start_image;
    void* exit;
    void* unload_image;
    Status (*exit_boot_services)(Handle image, uint64_t map_key);
    void* get_next_monotonic_count;
    Status (*stall)(uint64_t microseconds);
    Status (*set_watchdog_timer)(uint64_t timeout, uint64_t code, uint64_t size, Char16* data);
    void* connect_controller;
    void* disconnect_controller;
    void* open_protocol;
    void* close_protocol;
    void* open_protocol_information;
    void* protocols_per_handle;
    void* locate_handle_buffer;
    Status (*locate_protocol)(const Guid* protocol, void* registration, void** interface);
    // later slots (InstallMultipleProtocolInterfaces, CalculateCrc32, ...) are not used
};

struct RuntimeServices {    // UEFI Specification: EFI_RUNTIME_SERVICES (slots in table order)
    TableHeader hdr;
    void* get_time;
    void* set_time;
    void* get_wakeup_time;
    void* set_wakeup_time;
    void* set_virtual_address_map;
    void* convert_pointer;
    Status (*get_variable)(const Char16* name, const Guid* vendor, uint32_t* attributes,
                           uint64_t* data_size, void* data);
    void* get_next_variable_name;
    Status (*set_variable)(const Char16* name, const Guid* vendor, uint32_t attributes,
                           uint64_t data_size, const void* data);
    // later slots are not used
};

struct ConfigurationTable { // UEFI Specification: EFI_CONFIGURATION_TABLE
    Guid vendor_guid;
    void* vendor_table;
};

struct SystemTable {        // UEFI Specification: EFI_SYSTEM_TABLE
    TableHeader hdr;
    Char16* firmware_vendor;
    uint32_t firmware_revision;
    Handle console_in_handle;
    void* con_in;
    Handle console_out_handle;
    SimpleTextOutput* con_out;
    Handle standard_error_handle;
    SimpleTextOutput* std_err;
    RuntimeServices* runtime_services;
    BootServices* boot_services;
    uint64_t number_of_table_entries;
    ConfigurationTable* configuration_table;
};

// Configuration-table GUIDs (UEFI Specification, "EFI Configuration Table"; ACPI and SMBIOS GUIDs).
constexpr Guid kAcpi20TableGuid{0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81}};
constexpr Guid kAcpi10TableGuid{0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}};
constexpr Guid kSmbiosTableGuid{0xeb9d2d31, 0x2d88, 0x11d3, {0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d}};
constexpr Guid kSmbios3TableGuid{0xf2fd1544, 0x9794, 0x4a2c, {0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94}};

// Global variables such as SecureBoot and SetupMode (UEFI Specification, "Globally Defined Variables").
constexpr Guid kGlobalVariableGuid{0x8be4df61, 0x93ca, 0x11d2, {0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c}};
// db and dbx (UEFI Specification, "Secure Boot and Driver Signing": EFI_IMAGE_SECURITY_DATABASE_GUID).
constexpr Guid kImageSecurityDatabaseGuid{0xd719b2cb, 0x3d3a, 0x4596, {0xa3, 0xbc, 0xda, 0xd0, 0x0e, 0x67, 0x65, 0x6f}};
// Signature types inside an EFI_SIGNATURE_LIST.
constexpr Guid kCertX509Guid{0xa5c059a1, 0x94e4, 0x4aa7, {0x87, 0xb5, 0xab, 0x15, 0x5c, 0x2b, 0xf0, 0x72}};
constexpr Guid kCertSha256Guid{0xc1c41626, 0x504c, 0x4092, {0xac, 0xa9, 0x41, 0xf9, 0x36, 0x93, 0x43, 0x28}};

// --- protocols used by the loader in F3-15 ----------------------------------------------------
constexpr Guid kLoadedImageProtocolGuid{0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
constexpr Guid kSimpleFileSystemProtocolGuid{0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
constexpr Guid kFileInfoGuid{0x09576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
constexpr Guid kGraphicsOutputProtocolGuid{0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}};

struct LoadedImage {        // UEFI Specification: EFI_LOADED_IMAGE_PROTOCOL
    uint32_t revision;
    Handle parent_handle;
    SystemTable* system_table;
    Handle device_handle;   // the device (here: the ESP) the image was loaded from
    void* file_path;
    void* reserved;
    uint32_t load_options_size;
    void* load_options;
    void* image_base;
    uint64_t image_size;
    MemoryType image_code_type;
    MemoryType image_data_type;
    void* unload;
};

struct File {               // UEFI Specification: EFI_FILE_PROTOCOL
    uint64_t revision;
    Status (*open)(File* self, File** out, const Char16* name, uint64_t mode, uint64_t attributes);
    Status (*close)(File* self);
    void* del;
    Status (*read)(File* self, uint64_t* size, void* buffer);
    void* write;
    void* get_position;
    void* set_position;
    Status (*get_info)(File* self, const Guid* type, uint64_t* size, void* buffer);
};
constexpr uint64_t kFileModeRead = 1;

struct SimpleFileSystem {   // UEFI Specification: EFI_SIMPLE_FILE_SYSTEM_PROTOCOL
    uint64_t revision;
    Status (*open_volume)(SimpleFileSystem* self, File** root);
};

struct FileInfo {           // UEFI Specification: EFI_FILE_INFO (three 16-byte EFI_TIME fields)
    uint64_t size;
    uint64_t file_size;
    uint64_t physical_size;
    uint8_t times[48];
    uint64_t attribute;
    Char16 file_name[1];
};

struct GopModeInfo {        // UEFI Specification: EFI_GRAPHICS_OUTPUT_MODE_INFORMATION
    uint32_t version;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format;
    uint32_t pixel_information[4];
    uint32_t pixels_per_scan_line;
};

struct GopMode {            // UEFI Specification: EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE
    uint32_t max_mode;
    uint32_t mode;
    GopModeInfo* info;
    uint64_t size_of_info;
    uint64_t frame_buffer_base;
    uint64_t frame_buffer_size;
};

struct GraphicsOutput {     // UEFI Specification: EFI_GRAPHICS_OUTPUT_PROTOCOL
    void* query_mode;
    void* set_mode;
    void* blt;
    GopMode* mode;
};

}  // namespace efi
