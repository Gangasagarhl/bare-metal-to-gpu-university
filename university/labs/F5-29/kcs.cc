// kcs.cc - talk IPMI to a BMC from the host side ("in-band") through the KCS interface:
// find the BMC in the SMBIOS inventory (type 38), then send six requests and decode the
// replies: Get Device ID, Get Self Test Results, Get Chassis Status, Get SEL Info, Add SEL
// Entry + Get SEL Entry, and finally Chassis Control "power down".
// A UEFI application (build route of F3-10). The KCS handshake, the message layout and the
// command numbers were written from memory of the IPMI v2.0 specification ("KCS Interface",
// "IPMI Messaging", command tables); what the run shows is that QEMU's simulated BMC
// (ipmi-bmc-sim) accepted them. Check them in the specification before reusing this file.
#include "console.hpp"
#include "efi.hpp"

namespace {

uint8_t inb(uint16_t port)
{
    uint8_t v = 0;
    __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void outb(uint16_t port, uint8_t v)
{
    __asm__ volatile("outb %0, %1" : : "a"(v), "Nd"(port));
}

uint16_t u16(const uint8_t* p, int o) { return static_cast<uint16_t>(p[o] | p[o + 1] << 8); }
uint32_t u32(const uint8_t* p, int o) { return u16(p, o) | static_cast<uint32_t>(u16(p, o + 2)) << 16; }
uint64_t u64(const uint8_t* p, int o) { return u32(p, o) | static_cast<uint64_t>(u32(p, o + 4)) << 32; }

// KCS registers: data at the base port, status (read) / command (write) at base + 1.
uint16_t g_data = 0xca2;
uint16_t g_status = 0xca3;
constexpr uint8_t kObf = 0x01;           // output buffer full: a byte waits for the host
constexpr uint8_t kIbf = 0x02;           // input buffer full: the BMC has not taken our byte yet
constexpr uint8_t kWriteStart = 0x61;    // KCS control codes
constexpr uint8_t kWriteEnd = 0x62;
constexpr uint8_t kRead = 0x68;
enum State { kIdle = 0, kReadState = 1, kWriteState = 2, kErrorState = 3 };

int state() { return inb(g_status) >> 6; }

bool wait_ibf_clear()
{
    for (int i = 0; i < 1000000; ++i) {
        if ((inb(g_status) & kIbf) == 0) {
            return true;
        }
    }
    return false;
}

bool wait_obf_set()
{
    for (int i = 0; i < 1000000; ++i) {
        if ((inb(g_status) & kObf) != 0) {
            return true;
        }
    }
    return false;
}

void clear_obf()
{
    if ((inb(g_status) & kObf) != 0) {
        inb(g_data);
    }
}

// One request/response through the KCS state machine. Returns the response length,
// or a negative step number when the BMC did not behave as expected.
int transact(const uint8_t* req, int n, uint8_t* rsp, int max)
{
    if (!wait_ibf_clear()) { return -1; }
    clear_obf();
    outb(g_status, kWriteStart);
    if (!wait_ibf_clear() || state() != kWriteState) { return -2; }
    clear_obf();
    for (int i = 0; i < n - 1; ++i) {
        outb(g_data, req[i]);
        if (!wait_ibf_clear() || state() != kWriteState) { return -3; }
        clear_obf();
    }
    outb(g_status, kWriteEnd);
    if (!wait_ibf_clear() || state() != kWriteState) { return -4; }
    clear_obf();
    outb(g_data, req[n - 1]);
    int got = 0;
    for (;;) {
        if (!wait_ibf_clear()) { return -5; }
        const int s = state();
        if (s == kReadState) {
            if (!wait_obf_set()) { return -6; }
            const uint8_t b = inb(g_data);
            if (got < max) {
                rsp[got++] = b;
            }
            outb(g_data, kRead);
        } else if (s == kIdle) {
            if (wait_obf_set()) {
                inb(g_data);                     // the dummy byte that ends the read phase
            }
            return got;
        } else {
            return -7;
        }
    }
}

// Send netfn/cmd/data, print the raw exchange, return the response length (or < 0).
int ipmi(Console& con, const char* what, uint8_t netfn, uint8_t cmd,
         const uint8_t* data, int len, uint8_t* rsp)
{
    uint8_t req[32];
    req[0] = static_cast<uint8_t>(netfn << 2);   // LUN 0 in the low two bits
    req[1] = cmd;
    for (int i = 0; i < len; ++i) {
        req[2 + i] = data[i];
    }
    con.print(what);
    con.print("\n  request :");
    for (int i = 0; i < len + 2; ++i) {
        con.print(" ");
        con.hex(req[i], 2);
    }
    const int n = transact(req, len + 2, rsp, 32);
    con.print("\n  response:");
    for (int i = 0; i < n; ++i) {
        con.print(" ");
        con.hex(rsp[i], 2);
    }
    if (n < 3) {
        con.print("  (transfer failed at step ");
        con.dec(static_cast<uint64_t>(-n));
        con.print(")\n");
        return -1;
    }
    con.print("\n  netfn ");
    con.hex(rsp[0] >> 2, 2);
    con.print(" cmd ");
    con.hex(rsp[1], 2);
    con.print(" completion code ");
    con.hex(rsp[2], 2);
    con.print(rsp[2] == 0 ? " (ok)\n" : " (error)\n");
    return n;
}

const uint8_t* find_smbios_type(const uint8_t* ep, uint8_t type)
{
    const uint8_t* s = reinterpret_cast<const uint8_t*>(u64(ep, 16));
    const uint8_t* end = s + u32(ep, 12);
    while (s + 4 <= end && s[0] != 127) {
        if (s[0] == type) {
            return s;
        }
        const uint8_t* p = s + s[1];
        while (p + 1 < end && (p[0] != 0 || p[1] != 0)) {
            ++p;
        }
        s = p + 2;
    }
    return nullptr;
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    con.print("F5-29 kcs: start\n");
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        const efi::ConfigurationTable& e = st->configuration_table[i];
        if (!efi::same(e.vendor_guid, efi::kSmbios3TableGuid)) {
            continue;
        }
        const uint8_t* t38 = find_smbios_type(static_cast<const uint8_t*>(e.vendor_table), 38);
        if (t38 == nullptr) {
            con.print("SMBIOS: no type 38 (IPMI device information); using the default port\n");
            break;
        }
        const uint64_t base = u64(t38, 8);
        con.print("SMBIOS type 38: interface type ");
        con.dec(t38[4]);
        con.print(t38[4] == 1 ? " (KCS)" : "");
        con.print(", IPMI version ");
        con.dec(t38[5] >> 4);
        con.print(".");
        con.dec(t38[5] & 0xf);
        con.print(", BMC address ");
        con.hex(t38[6], 2);
        con.print(", base ");
        con.hex(base, 8);
        con.print((base & 1) != 0 ? " (I/O space)\n" : " (memory space)\n");
        if ((base & 1) != 0) {
            g_data = static_cast<uint16_t>(base & ~1ull);
            g_status = static_cast<uint16_t>(g_data + 1);
        }
    }
    con.print("KCS data port ");
    con.hex(g_data, 4);
    con.print(", status port ");
    con.hex(g_status, 4);
    con.print(", status now ");
    con.hex(inb(g_status), 2);
    con.print("\n");

    uint8_t r[32];
    if (ipmi(con, "Get Device ID (netfn App 0x06, cmd 0x01)", 0x06, 0x01, nullptr, 0, r) >= 14) {
        con.print("  device ID ");
        con.hex(r[3], 2);
        con.print(", firmware ");
        con.dec(r[5] & 0x7f);
        con.print(".");
        con.hex(r[6], 2);
        con.print(", IPMI ");
        con.dec(r[7] & 0xf);
        con.print(".");
        con.dec(r[7] >> 4);
        con.print(", manufacturer ");
        con.hex(r[9] | r[10] << 8 | r[11] << 16, 6);
        con.print(", product ");
        con.hex(u16(r, 12), 4);
        con.print("\n");
    }
    if (ipmi(con, "Get Self Test Results (netfn App 0x06, cmd 0x04)", 0x06, 0x04, nullptr, 0, r) >= 5) {
        con.print(r[3] == 0x55 ? "  self test: no error\n" : "  self test: see the code above\n");
    }
    if (ipmi(con, "Get Chassis Status (netfn Chassis 0x00, cmd 0x01)", 0x00, 0x01, nullptr, 0, r) >= 4) {
        con.print((r[3] & 1) != 0 ? "  system power: on\n" : "  system power: off\n");
    }
    if (ipmi(con, "Get SEL Info (netfn Storage 0x0a, cmd 0x40)", 0x0a, 0x40, nullptr, 0, r) >= 8) {
        con.print("  SEL entries ");
        con.dec(u16(r, 4));
        con.print(", free bytes ");
        con.dec(u16(r, 6));
        con.print("\n");
    }
    // A 16-byte system event record: record ID (filled in by the BMC), type 0x02, time
    // stamp (filled in by the BMC), generator ID, event message revision, sensor type,
    // sensor number, event type, three event data bytes. The values describe a made-up
    // "temperature upper critical going high" event for this exercise.
    const uint8_t event[16] = {0x00, 0x00, 0x02, 0, 0, 0, 0, 0x41, 0x00, 0x04,
                               0x01, 0x30, 0x01, 0x09, 0xff, 0xff};
    uint16_t id = 0;
    if (ipmi(con, "Add SEL Entry (netfn Storage 0x0a, cmd 0x44)", 0x0a, 0x44, event, 16, r) >= 5) {
        id = u16(r, 3);
        con.print("  stored as record ID ");
        con.hex(id, 4);
        con.print("\n");
    }
    const uint8_t get[6] = {0x00, 0x00, static_cast<uint8_t>(id), static_cast<uint8_t>(id >> 8), 0x00, 0xff};
    if (ipmi(con, "Get SEL Entry (netfn Storage 0x0a, cmd 0x43)", 0x0a, 0x43, get, 6, r) >= 21) {
        con.print("  record ");
        con.hex(u16(r, 5), 4);
        con.print(" type ");
        con.hex(r[7], 2);
        con.print(" time stamp ");
        con.dec(u32(r, 8));
        con.print(" sensor type ");
        con.hex(r[15], 2);
        con.print(" sensor ");
        con.hex(r[16], 2);
        con.print("\n");
    }
    ipmi(con, "Get SEL Info again", 0x0a, 0x40, nullptr, 0, r);
    const uint8_t power_down[1] = {0x00};
    con.print("F5-29 kcs: asking the BMC to power the system down\n");
    ipmi(con, "Chassis Control (netfn Chassis 0x00, cmd 0x02, data 0x00 = power down)",
         0x00, 0x02, power_down, 1, r);
    for (int i = 0; i < 100; ++i) {
        st->boot_services->stall(100000);       // 0.1 s: give the BMC time to act
    }
    con.print("F5-29 kcs: still running after 10 s, so the power-down did not happen\n");
    qemu_exit(0x10);
    return efi::kSuccess;
}
