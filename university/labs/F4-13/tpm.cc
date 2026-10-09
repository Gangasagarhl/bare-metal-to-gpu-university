// tpm.cc - DR302 F4-13: TIS FIFO driver and TPM 2.0 commands (see tpm.h).
#include "tpm.h"
#include "kbase.h"
#include "../F4-08/intr.h"

namespace tpm {
bool trace = false;
}

namespace {
uint8_t r8(uint32_t o) { return mmio_read<uint8_t>(tis::BASE + o); }
uint32_t r32(uint32_t o) { return mmio_read<uint32_t>(tis::BASE + o); }
void w8(uint32_t o, uint8_t v) { mmio_write<uint8_t>(tis::BASE + o, v); }
void w32(uint32_t o, uint32_t v) { mmio_write<uint32_t>(tis::BASE + o, v); }

bool wait_sts(uint32_t mask, uint32_t want, uint32_t ms)
{
    const uint64_t end = clock::ms() + ms;
    while ((r32(tis::STS) & mask) != want)
        if (clock::ms() >= end) return false;
    return true;
}
uint32_t burst() { return (r32(tis::STS) >> 8) & 0xFFFF; }   // bytes the FIFO takes / gives now

// Big-endian writers: every TPM 2.0 command field is big-endian.
struct Out {
    uint8_t* b;
    uint32_t n;
    void u8(uint8_t v) { b[n++] = v; }
    void u16(uint16_t v) { u8(static_cast<uint8_t>(v >> 8)); u8(static_cast<uint8_t>(v)); }
    void u32(uint32_t v) { u16(static_cast<uint16_t>(v >> 16)); u16(static_cast<uint16_t>(v)); }
    void bytes(const uint8_t* p, uint32_t k) { for (uint32_t i = 0; i < k; ++i) u8(p[i]); }
    void finish() { b[2] = static_cast<uint8_t>(n >> 24); b[3] = static_cast<uint8_t>(n >> 16);
                    b[4] = static_cast<uint8_t>(n >> 8); b[5] = static_cast<uint8_t>(n); }   // commandSize
};
uint32_t be32(const uint8_t* p) { return (uint32_t{p[0]} << 24) | (uint32_t{p[1]} << 16) | (uint32_t{p[2]} << 8) | p[3]; }
uint16_t be16(const uint8_t* p) { return static_cast<uint16_t>((p[0] << 8) | p[1]); }

void dump(const char* dir, const uint8_t* p, uint32_t n)
{
    kprintf("tpm: %s %u bytes:", dir, n);
    for (uint32_t i = 0; i < n; ++i) kprintf("%s%02x", (i % 32 == 0) ? "\n     " : " ", p[i]);
    kprintf("\n");
}

uint8_t g_cmd[256], g_resp[512];
}  // namespace

namespace tpm {
bool probe()
{
    const uint32_t id = r32(tis::DID_VID);
    if (id == 0 || id == 0xFFFFFFFFu) {
        kprintf("tis: DID_VID reads 0x%08x at 0x%x: no TPM there\n", id, static_cast<uint32_t>(tis::BASE + tis::DID_VID));
        return false;
    }
    w8(tis::ACCESS, tis::ACCESS_REQUEST);                 // ask for locality 0
    const uint64_t end = clock::ms() + 750;
    while ((r8(tis::ACCESS) & (tis::ACCESS_VALID | tis::ACCESS_ACTIVE)) != (tis::ACCESS_VALID | tis::ACCESS_ACTIVE))
        if (clock::ms() >= end) { kprintf("tis: locality 0 not granted\n"); return false; }
    const uint32_t sts = r32(tis::STS);
    kprintf("tis: DID_VID 0x%08x (vendor 0x%04x), RID 0x%02x, INTERFACE_ID 0x%08x, ACCESS 0x%02x, STS 0x%08x "
            "(family bits 27-26 = %u: %s)\n", id, id & 0xFFFF, r8(tis::RID), r32(tis::INTERFACE_ID), r8(tis::ACCESS), sts,
            (sts >> 26) & 3, ((sts >> 26) & 3) == 1 ? "TPM 2.0" : "TPM 1.2 or unknown");
    return true;
}

uint32_t transmit(const uint8_t* cmd, uint32_t len, uint8_t* resp, uint32_t cap)
{
    if (trace) dump("command", cmd, len);
    w32(tis::STS, tis::STS_COMMAND_READY);                // 1. idle -> ready
    if (!wait_sts(tis::STS_COMMAND_READY, tis::STS_COMMAND_READY, 750)) { kprintf("tis: not ready\n"); return 0; }
    uint32_t sent = 0;                                    // 2. bytes into the FIFO, burstCount at a time
    while (sent < len) {
        uint32_t b = burst();
        if (b == 0) continue;
        while (b-- && sent < len) w8(tis::DATA_FIFO, cmd[sent++]);
        if (!wait_sts(tis::STS_VALID, tis::STS_VALID, 750)) return 0;
        const bool expect = r32(tis::STS) & tis::STS_EXPECT;
        if (sent < len && !expect) { kprintf("tis: TPM stopped expecting data after %u of %u bytes\n", sent, len); return 0; }
        if (sent == len && expect) { kprintf("tis: TPM still expects data after %u bytes\n", len); return 0; }
    }
    w32(tis::STS, tis::STS_GO);                           // 3. execute
    if (!wait_sts(tis::STS_VALID | tis::STS_DATA_AVAIL, tis::STS_VALID | tis::STS_DATA_AVAIL, 2000)) {
        kprintf("tis: no response\n");
        return 0;
    }
    uint32_t got = 0, total = 10;                         // 4. header first: it holds the size
    while (got < total && got < cap) {
        uint32_t b = burst();
        if (b == 0) continue;
        while (b-- && got < total && got < cap) resp[got++] = r8(tis::DATA_FIFO);
        if (got == 10) total = be32(resp + 2);
    }
    w32(tis::STS, tis::STS_COMMAND_READY);                // 5. back to idle
    if (trace) dump("response", resp, got);
    return got;
}

uint32_t response_code(const uint8_t* resp) { return be32(resp + 6); }

const char* rc_name(uint32_t rc)
{
    switch (rc) {
    case 0x000: return "TPM_RC_SUCCESS";
    case 0x100: return "TPM_RC_INITIALIZE (TPM2_Startup not sent yet)";
    case 0x101: return "TPM_RC_FAILURE";
    case 0x143: return "TPM_RC_COMMAND_CODE (command not implemented)";
    case 0x084: return "TPM_RC_VALUE";
    case 0xFFFFFFFFu: return "no response (interface error)";
    default: return "other (decode with the TPM_RC tables)";
    }
}

uint32_t pcr_read(uint32_t index, uint8_t out[32])
{
    Out o{g_cmd, 0};
    o.u16(tpm2::ST_NO_SESSIONS); o.u32(0); o.u32(tpm2::CC_PCR_READ);
    o.u32(1);                                             // TPML_PCR_SELECTION: one bank
    o.u16(tpm2::ALG_SHA256); o.u8(3);                     // 3 bytes of bitmap = PCR 0..23
    o.u8(index < 8 ? 1u << index : 0); o.u8(index >= 8 && index < 16 ? 1u << (index - 8) : 0);
    o.u8(index >= 16 ? 1u << (index - 16) : 0);
    o.finish();
    const uint32_t n = transmit(g_cmd, o.n, g_resp, sizeof g_resp);
    if (n < 10) return 0xFFFFFFFFu;
    if (response_code(g_resp)) return response_code(g_resp);
    // pcrUpdateCounter(4), TPML_PCR_SELECTION(4 + 2 + 1 + 3), TPML_DIGEST: count(4), size(2), bytes
    const uint8_t* p = g_resp + 10 + 4 + 10;
    if (be32(p) != 1 || be16(p + 4) != 32) return 0xFFFFFFFEu;
    memcpy(out, p + 6, 32);
    return 0;
}

uint32_t pcr_extend(uint32_t index, const uint8_t digest[32])
{
    Out o{g_cmd, 0};
    o.u16(tpm2::ST_SESSIONS); o.u32(0); o.u32(tpm2::CC_PCR_EXTEND);
    o.u32(index);                                         // the PCR handle is the PCR number
    o.u32(9);                                             // authorizationSize
    o.u32(tpm2::RS_PW); o.u16(0); o.u8(0); o.u16(0);      // password session: no nonce, attrs 0, empty hmac
    o.u32(1);                                             // TPML_DIGEST_VALUES: one digest
    o.u16(tpm2::ALG_SHA256); o.bytes(digest, 32);
    o.finish();
    const uint32_t n = transmit(g_cmd, o.n, g_resp, sizeof g_resp);
    return n < 10 ? 0xFFFFFFFFu : response_code(g_resp);
}

uint32_t get_random(uint8_t* out, uint16_t n)
{
    Out o{g_cmd, 0};
    o.u16(tpm2::ST_NO_SESSIONS); o.u32(0); o.u32(tpm2::CC_GET_RANDOM); o.u16(n);
    o.finish();
    const uint32_t got = transmit(g_cmd, o.n, g_resp, sizeof g_resp);
    if (got < 12 || response_code(g_resp)) return got < 10 ? 0xFFFFFFFFu : response_code(g_resp);
    const uint16_t k = be16(g_resp + 10);
    memcpy(out, g_resp + 12, k < n ? k : n);
    return 0;
}
}  // namespace tpm
