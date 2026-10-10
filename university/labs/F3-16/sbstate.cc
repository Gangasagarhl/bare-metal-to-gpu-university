// sbstate.cc - print the firmware's Secure Boot state from its global variables
// (SecureBoot, SetupMode) and whether a Platform Key (PK) is enrolled. A UEFI application;
// it only reads variables. Variable names and the vendor GUID are those of the UEFI
// Specification's "Globally Defined Variables" (written from memory, pending verification).
#include "console.hpp"
#include "efi.hpp"

namespace {

void show(Console& con, efi::RuntimeServices* rt, const efi::Char16* name, const char* label)
{
    uint8_t data[8] = {};
    uint64_t size = sizeof data;
    uint32_t attributes = 0;
    const efi::Status s = rt->get_variable(name, &efi::kGlobalVariableGuid, &attributes, &size, data);
    con.print(label);
    if (s == efi::kSuccess) {
        con.print(" = ");
        con.dec(data[0]);
        con.print(" (");
        con.dec(size);
        con.print(" byte(s), attributes ");
        con.hex(attributes, 2);
        con.print(")\n");
    } else if (s == efi::kBufferTooSmall) {
        con.print(": present, ");
        con.dec(size);
        con.print(" bytes\n");
    } else if (s == efi::kNotFound) {
        con.print(": not present\n");
    } else {
        con.print(": status ");
        con.hex(s);
        con.print("\n");
    }
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    con.print("F3-16 sbstate: Secure Boot variables\n");
    show(con, st->runtime_services, u"SecureBoot", "  SecureBoot");
    show(con, st->runtime_services, u"SetupMode", "  SetupMode");
    show(con, st->runtime_services, u"PK", "  PK (Platform Key)");
    con.print("F3-16 sbstate: done\n");
    qemu_exit(0x10);
    return efi::kSuccess;
}
