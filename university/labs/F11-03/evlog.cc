// evlog.cc - F11-03 Listing 3: read the firmware's TPM event log through the EFI TCG2 protocol,
// print every event, and replay the SHA-256 digests into PCRs 0-7.
#include "console.hpp"
#include "efi.hpp"
#include "sha256.h"

namespace {

struct Tcg2 {   // EFI_TCG2_PROTOCOL (TCG EFI Protocol Specification; slots in order, from memory)
    void* get_capability;
    efi::Status (*get_event_log)(Tcg2* self, uint32_t format, uint64_t* location, uint64_t* last_entry,
                                 uint8_t* truncated);
    // later slots (HashLogExtendEvent, SubmitCommand, ...) are not used
};
constexpr efi::Guid kTcg2ProtocolGuid{0x607f766c, 0x7455, 0x42be, {0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f}};
constexpr uint32_t kLogFormatTcg2 = 2;
constexpr uint32_t kEvNoAction = 3;
constexpr uint16_t kAlgSha256 = 0x000b;

uint32_t le32(const uint8_t* p)
{
    return uint32_t{p[0]} | (uint32_t{p[1]} << 8) | (uint32_t{p[2]} << 16) | (uint32_t{p[3]} << 24);
}
uint16_t le16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }

void hex_bytes(Console& con, const uint8_t* p, int n)
{
    char text[2 * 32 + 1] = {};
    for (int i = 0; i < n && i < 32; ++i) {
        text[2 * i] = "0123456789abcdef"[p[i] >> 4];
        text[2 * i + 1] = "0123456789abcdef"[p[i] & 0xf];
    }
    con.print(text);
}

const char* type_name(uint32_t t)   // event type names from memory of the TCG PC Client PFP
{
    switch (t) {
    case 0x00000001: return "EV_POST_CODE";
    case 0x00000003: return "EV_NO_ACTION";
    case 0x00000004: return "EV_SEPARATOR";
    case 0x00000008: return "EV_S_CRTM_VERSION";
    case 0x80000001: return "EV_EFI_VARIABLE_DRIVER_CONFIG";
    case 0x80000002: return "EV_EFI_VARIABLE_BOOT";
    case 0x80000003: return "EV_EFI_BOOT_SERVICES_APPLICATION";
    case 0x80000006: return "EV_EFI_GPT_EVENT";
    case 0x80000007: return "EV_EFI_ACTION";
    case 0x80000008: return "EV_EFI_PLATFORM_FIRMWARE_BLOB";
    case 0x80000009: return "EV_EFI_HANDOFF_TABLES";
    case 0x8000000a: return "EV_EFI_PLATFORM_FIRMWARE_BLOB2";
    case 0x800000e0: return "EV_EFI_VARIABLE_AUTHORITY";
    default: return nullptr;
    }
}

// For variable events the event data holds a GUID, two 64-bit lengths and the UTF-16 name.
void print_variable_name(Console& con, const uint8_t* ev, uint32_t size)
{
    if (size < 32) { return; }
    const uint32_t chars = le32(ev + 16);
    char name[24] = {};
    for (uint32_t i = 0; i < chars && i < 23 && 32 + 2 * i + 1 < size; ++i) {
        name[i] = static_cast<char>(ev[32 + 2 * i]);
    }
    con.print(" ");
    con.print(name);
}

uint8_t g_pcr[8][32];

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    con.print("F11-03 evlog: the firmware's TPM event log\n");
    Tcg2* tcg2 = nullptr;
    if (st->boot_services->locate_protocol(&kTcg2ProtocolGuid, nullptr, reinterpret_cast<void**>(&tcg2)) != efi::kSuccess) {
        con.print("  no TCG2 protocol: this machine has no TPM (or the firmware has no TPM support)\n");
        qemu_exit(0x11);
        return efi::kSuccess;
    }
    uint64_t first = 0, last = 0;
    uint8_t truncated = 0;
    const efi::Status s = tcg2->get_event_log(tcg2, kLogFormatTcg2, &first, &last, &truncated);
    if (s != efi::kSuccess || first == 0) {
        con.print("  GetEventLog failed\n");
        qemu_exit(0x12);
        return efi::kSuccess;
    }
    const uint8_t* p = reinterpret_cast<const uint8_t*>(first);
    // first entry: the legacy-format header event (PCR, type, 20-byte digest, size, data)
    const uint32_t spec_size = le32(p + 28);
    const uint8_t* spec = p + 32;
    const uint32_t nalg = le32(spec + 24);
    uint16_t digest_size[8] = {};
    uint16_t alg_id[8] = {};
    con.print("  header event: \"");
    con.print(reinterpret_cast<const char*>(spec));
    con.print("\", ");
    con.dec(nalg);
    con.print(" digest algorithm(s)\n");
    for (uint32_t i = 0; i < nalg && i < 8; ++i) {
        alg_id[i] = le16(spec + 28 + 4 * i);
        digest_size[i] = le16(spec + 30 + 4 * i);
    }
    p += 32 + spec_size;
    const uint8_t* end = reinterpret_cast<const uint8_t*>(last);
    int n = 0;
    con.print("  #   PCR type                              sha256 (first 8 bytes)  data\n");
    while (p <= end) {
        const uint32_t pcr = le32(p), type = le32(p + 4), count = le32(p + 8);
        const uint8_t* q = p + 12;
        const uint8_t* sha = nullptr;
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t alg = le16(q);
            uint16_t size = 0;
            for (uint32_t j = 0; j < nalg && j < 8; ++j) { if (alg_id[j] == alg) { size = digest_size[j]; } }
            if (alg == kAlgSha256) { sha = q + 2; }
            q += 2 + size;
        }
        const uint32_t ev_size = le32(q);
        const uint8_t* ev = q + 4;
        con.print("  ");
        if (n < 10) { con.print(" "); }
        con.dec(n++);
        con.print("  ");
        con.dec(pcr);
        con.print("   ");
        const char* name = type_name(type);
        int len = 10;
        if (name != nullptr) {
            con.print(name);
            for (len = 0; name[len] != '\0'; ++len) {
            }
        } else {
            con.hex(type, 8);
        }
        for (int pad = len; pad < 34; ++pad) { con.print(" "); }
        if (sha != nullptr) { hex_bytes(con, sha, 8); } else { con.print("(no sha256)     "); }
        con.print("        ");
        con.dec(ev_size);
        con.print(" B");
        if (type == 0x80000001 || type == 0x80000002 || type == 0x800000e0) { print_variable_name(con, ev, ev_size); }
        con.print("\n");
        if (type != kEvNoAction && sha != nullptr && pcr < 8) {   // replay: PCR = SHA-256(PCR || digest)
            Sha256 h;
            h.update(g_pcr[pcr], 32);
            h.update(sha, 32);
            h.finish(g_pcr[pcr]);
        }
        p = ev + ev_size;
    }
    con.print("  replayed PCR values:\n");
    for (int i = 0; i < 8; ++i) {
        con.print("    PCR ");
        con.dec(i);
        con.print(" ");
        hex_bytes(con, g_pcr[i], 32);
        con.print("\n");
    }
    qemu_exit(0x10);
    return efi::kSuccess;
}
