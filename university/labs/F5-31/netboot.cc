// netboot.cc - the program a server boots over the network in the F5-31 lab. It prints
// (1) how it was loaded: the device path of the device its image came from, decoded node by
// node, and (2) the server's inventory from SMBIOS as "inventory: key=value" lines that a
// script can collect from many servers. A UEFI application (build route of F3-10).
// The device-path node numbers and the SMBIOS offsets were written from memory of the UEFI and
// SMBIOS specifications; the lab compares them with OVMF's own text and QEMU's options.
#include "console.hpp"
#include "efi.hpp"

namespace {

constexpr efi::Guid kDevicePathGuid{0x09576e91, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

uint16_t u16(const uint8_t* p, int o) { return static_cast<uint16_t>(p[o] | p[o + 1] << 8); }
uint32_t u32(const uint8_t* p, int o) { return u16(p, o) | static_cast<uint32_t>(u16(p, o + 2)) << 16; }
uint64_t u64(const uint8_t* p, int o) { return u32(p, o) | static_cast<uint64_t>(u32(p, o + 4)) << 32; }

void hex2(Console& con, uint8_t b)
{
    char s[3] = {"0123456789abcdef"[b >> 4], "0123456789abcdef"[b & 0xf], '\0'};
    con.print(s);
}

void ipv4(Console& con, const uint8_t* a)
{
    for (int i = 0; i < 4; ++i) {
        con.dec(a[i]);
        con.print(i < 3 ? "." : "");
    }
}

// Walk a device path: nodes of (type, subtype, 16-bit length), ended by type 0x7f.
void device_path(Console& con, const uint8_t* n)
{
    for (int count = 0; count < 16 && n[0] != 0x7f; ++count) {
        const uint16_t len = u16(n, 2);
        con.print("  node type ");
        con.dec(n[0]);
        con.print(" subtype ");
        con.dec(n[1]);
        if (n[0] == 1 && n[1] == 1) {
            con.print(": PCI device ");
            con.hex(n[5], 2);
            con.print(" function ");
            con.hex(n[4], 2);
        } else if (n[0] == 3 && n[1] == 11) {
            con.print(": MAC address ");
            for (int i = 0; i < 6; ++i) {
                hex2(con, n[4 + i]);
            }
        } else if (n[0] == 3 && n[1] == 12) {
            con.print(": IPv4, local ");
            ipv4(con, n + 4);
            con.print(", remote ");
            ipv4(con, n + 8);
        } else if (n[0] == 2 && n[1] == 1) {
            con.print(": ACPI root");
        }
        con.print("\n");
        if (len < 4) {
            break;                                  // damaged node
        }
        n += len;
    }
}

const char* str(const uint8_t* s, uint8_t index)    // SMBIOS string number "index" (1-based)
{
    if (index == 0) {
        return "(none)";
    }
    const char* p = reinterpret_cast<const char*>(s + s[1]);
    for (uint8_t i = 1; i < index && *p != '\0'; ++i) {
        while (*p != '\0') {
            ++p;
        }
        ++p;
    }
    return p;
}

void kv(Console& con, const char* key, const char* value)
{
    con.print("inventory: ");
    con.print(key);
    con.print("=");
    con.print(value);
    con.print("\n");
}

void inventory(Console& con, const uint8_t* ep)
{
    const uint8_t* s = reinterpret_cast<const uint8_t*>(u64(ep, 16));
    const uint8_t* end = s + u32(ep, 12);
    int sockets = 0;
    uint64_t memory_mib = 0;
    while (s + 4 <= end && s[0] != 127) {
        if (s[0] == 0) {
            kv(con, "firmware.vendor", str(s, s[4]));
            kv(con, "firmware.version", str(s, s[5]));
        } else if (s[0] == 1) {
            kv(con, "system.manufacturer", str(s, s[4]));
            kv(con, "system.product", str(s, s[5]));
            kv(con, "system.serial", str(s, s[7]));
            con.print("inventory: system.uuid=");
            for (int i = 0; i < 16; ++i) {
                hex2(con, s[8 + i]);                // raw byte order, as stored
            }
            con.print("\n");
        } else if (s[0] == 3) {
            kv(con, "chassis.serial", str(s, s[7]));
            kv(con, "chassis.asset_tag", str(s, s[8]));
        } else if (s[0] == 4) {
            ++sockets;
        } else if (s[0] == 17) {
            const uint16_t size = u16(s, 0x0c);
            memory_mib += (size & 0x8000) != 0 ? (size & 0x7fff) / 1024 : size;
        }
        const uint8_t* p = s + s[1];
        while (p + 1 < end && (p[0] != 0 || p[1] != 0)) {
            ++p;
        }
        s = p + 2;
    }
    con.print("inventory: cpu.sockets=");
    con.dec(static_cast<uint64_t>(sockets));
    con.print("\ninventory: memory.mib=");
    con.dec(memory_mib);
    con.print("\n");
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle image, efi::SystemTable* st)
{
    Console con(st->con_out);
    con.print("F5-31 netboot: started\n");
    void* li = nullptr;
    void* dp = nullptr;
    if (st->boot_services->handle_protocol(image, &efi::kLoadedImageProtocolGuid, &li) == efi::kSuccess &&
        st->boot_services->handle_protocol(static_cast<efi::LoadedImage*>(li)->device_handle,
                                           &kDevicePathGuid, &dp) == efi::kSuccess) {
        con.print("loaded from device path:\n");
        device_path(con, static_cast<const uint8_t*>(dp));
    } else {
        con.print("device path of the boot device: not available\n");
    }
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        if (efi::same(st->configuration_table[i].vendor_guid, efi::kSmbios3TableGuid)) {
            inventory(con, static_cast<const uint8_t*>(st->configuration_table[i].vendor_table));
        }
    }
    con.print("F5-31 netboot: done\n");
    qemu_exit(0x10);
    return efi::kSuccess;
}
