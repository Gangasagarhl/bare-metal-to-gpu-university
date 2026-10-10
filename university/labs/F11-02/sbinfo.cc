// sbinfo.cc - F11-02 Listing 4: print the Secure Boot state and list the key databases PK, KEK,
// db and dbx: every entry's type and, for certificates, the SHA-256 fingerprint of the DER bytes
// (the same value `openssl x509 -fingerprint -sha256` prints), so a listing can be matched
// against the key files. A UEFI application; it only reads variables.
#include "console.hpp"
#include "efi.hpp"
#include "sha256.h"

namespace {

uint8_t g_buffer[16384];

void hex_bytes(Console& con, const uint8_t* p, int n)
{
    char text[2 * 32 + 1] = {};
    for (int i = 0; i < n && i < 32; ++i) {
        text[2 * i] = "0123456789abcdef"[p[i] >> 4];
        text[2 * i + 1] = "0123456789abcdef"[p[i] & 0xf];
    }
    con.print(text);
}

uint32_t le32(const uint8_t* p)
{
    return uint32_t{p[0]} | (uint32_t{p[1]} << 8) | (uint32_t{p[2]} << 16) | (uint32_t{p[3]} << 24);
}

void show_flag(Console& con, efi::RuntimeServices* rt, const efi::Char16* name, const char* label)
{
    uint8_t value = 0xff;
    uint64_t size = 1;
    uint32_t attributes = 0;
    const efi::Status s = rt->get_variable(name, &efi::kGlobalVariableGuid, &attributes, &size, &value);
    con.print(label);
    if (s == efi::kSuccess) {
        con.print(" = ");
        con.dec(value);
        con.print("\n");
    } else {
        con.print(": not present\n");
    }
}

// Walks the EFI_SIGNATURE_LISTs in one variable: GUID type, list size, header size, entry size,
// then entries of (16-byte owner GUID, signature data).
void list_db(Console& con, efi::RuntimeServices* rt, const efi::Char16* name, const efi::Guid* vendor,
             const char* label)
{
    uint64_t size = sizeof g_buffer;
    uint32_t attributes = 0;
    const efi::Status s = rt->get_variable(name, vendor, &attributes, &size, g_buffer);
    con.print(label);
    if (s != efi::kSuccess) {
        con.print(": empty (not present)\n");
        return;
    }
    con.print(": ");
    con.dec(size);
    con.print(" bytes\n");
    uint64_t off = 0;
    while (off + 28 <= size) {
        const efi::Guid* type = reinterpret_cast<const efi::Guid*>(g_buffer + off);
        const uint32_t list_size = le32(g_buffer + off + 16);
        const uint32_t header_size = le32(g_buffer + off + 20);
        const uint32_t entry_size = le32(g_buffer + off + 24);
        if (list_size < 28 || entry_size <= 16 || off + list_size > size) {
            con.print("    malformed list\n");
            return;
        }
        for (uint64_t e = off + 28 + header_size; e + entry_size <= off + list_size; e += entry_size) {
            const uint8_t* body = g_buffer + e + 16;
            const uint32_t body_size = entry_size - 16;
            if (efi::same(*type, efi::kCertX509Guid)) {
                uint8_t digest[32];
                Sha256 h;
                h.update(body, body_size);
                h.finish(digest);
                con.print("    X.509 certificate, ");
                con.dec(body_size);
                con.print(" bytes, SHA-256 fingerprint ");
                hex_bytes(con, digest, 8);
                con.print("...\n");
            } else if (efi::same(*type, efi::kCertSha256Guid)) {
                con.print("    SHA-256 image hash ");
                hex_bytes(con, body, 8);
                con.print("...\n");
            } else {
                con.print("    other signature type\n");
            }
        }
        off += list_size;
    }
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    efi::RuntimeServices* rt = st->runtime_services;
    con.print("F11-02 sbinfo: this program is running\n");
    show_flag(con, rt, u"SecureBoot", "  SecureBoot");
    show_flag(con, rt, u"SetupMode", "  SetupMode");
    list_db(con, rt, u"PK", &efi::kGlobalVariableGuid, "  PK ");
    list_db(con, rt, u"KEK", &efi::kGlobalVariableGuid, "  KEK");
    list_db(con, rt, u"db", &efi::kImageSecurityDatabaseGuid, "  db ");
    list_db(con, rt, u"dbx", &efi::kImageSecurityDatabaseGuid, "  dbx");
    qemu_exit(0x10);
    return efi::kSuccess;
}
