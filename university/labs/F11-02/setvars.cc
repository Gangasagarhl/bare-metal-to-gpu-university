// setvars.cc - F11-02 Listing 3: apply signed Secure Boot variable updates (made by mkvars.py)
// with the runtime service SetVariable, report what the firmware answered, then the new state.
// A UEFI application (clang, x86_64-unknown-windows target, lld-link), built with a different
// updates.inc for each step of the lab.
#include "console.hpp"
#include "efi.hpp"

namespace {

struct Update {
    const efi::Char16* name;
    const efi::Guid* vendor;
    uint32_t attributes;
    uint64_t size;
    const uint8_t* data;
};

#include "updates.inc"

void status_text(Console& con, efi::Status s)
{
    switch (s) {
    case efi::kSuccess: con.print("Success"); break;
    case efi::kSecurityViolation: con.print("Security Violation"); break;
    case efi::kWriteProtected: con.print("Write Protected"); break;
    case efi::kInvalidParameter: con.print("Invalid Parameter"); break;
    case efi::kOutOfResources: con.print("Out of Resources"); break;
    default: con.print("status "); con.hex(s);
    }
}

void print_name(Console& con, const efi::Char16* name)
{
    char text[16] = {};
    for (int i = 0; i < 15 && name[i] != 0; ++i) {
        text[i] = static_cast<char>(name[i]);
    }
    con.print(text);
}

void show_flag(Console& con, efi::RuntimeServices* rt, const efi::Char16* name)
{
    uint8_t value = 0xff;
    uint64_t size = 1;
    uint32_t attributes = 0;
    const efi::Status s = rt->get_variable(name, &efi::kGlobalVariableGuid, &attributes, &size, &value);
    con.print("  ");
    print_name(con, name);
    if (s == efi::kSuccess) {
        con.print(" = ");
        con.dec(value);
        con.print("\n");
    } else {
        con.print(": ");
        status_text(con, s);
        con.print("\n");
    }
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    efi::RuntimeServices* rt = st->runtime_services;
    con.print("F11-02 setvars: state before\n");
    show_flag(con, rt, u"SetupMode");
    show_flag(con, rt, u"SecureBoot");
    for (const Update& u : kUpdates) {
        const efi::Status s = rt->set_variable(u.name, u.vendor, u.attributes, u.size, u.data);
        con.print("  SetVariable(");
        print_name(con, u.name);
        con.print(", attributes ");
        con.hex(u.attributes, 2);
        con.print(", ");
        con.dec(u.size);
        con.print(" bytes) -> ");
        status_text(con, s);
        con.print("\n");
    }
    con.print("F11-02 setvars: state after\n");
    show_flag(con, rt, u"SetupMode");
    show_flag(con, rt, u"SecureBoot");
    qemu_exit(0x10);
    return efi::kSuccess;
}
