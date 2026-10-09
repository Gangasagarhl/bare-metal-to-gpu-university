// f413_main.cc - DR302 F4-13 lab kernel (milestone C13): measured boot, checked.
//   1. ACPI TPM2 table: where the TPM is and where the firmware's event log lives
//   2. TIS driver: locality 0, identity registers
//   3. Event log: parse SeaBIOS's crypto-agile log, replay it in software, compare with
//      the TPM's PCR 0-7 (read with TPM2_PCR_Read)
//   4. Our own measurement: hash a "kernel command line", extend PCR 8, log the event,
//      print an attestation report (log + PCR values) for the host verifier (verify.cc)
// Without a TPM the kernel says so and stops after step 2.
// The forensic build (-DF413_SIZEOF_BUG) measures the command line with sizeof, not strlen.
#include "kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-08/intr.h"
#include "sha256.h"
#include "tpm.h"

namespace {
template <typename T> T le(const uint8_t* p)
{
    T v;
    memcpy(&v, p, sizeof v);              // the event log is little-endian, unlike TPM commands
    return v;
}
void hex(const uint8_t* p, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) kprintf("%02x", p[i]);
}
const char* event_name(uint32_t t)
{
    switch (t) {
    case 0x1: return "EV_POST_CODE";
    case 0x3: return "EV_NO_ACTION";
    case 0x4: return "EV_SEPARATOR";
    case 0x5: return "EV_ACTION";
    case 0x6: return "EV_EVENT_TAG";
    case 0x7: return "EV_S_CRTM_CONTENTS";
    case 0x8: return "EV_S_CRTM_VERSION";
    case 0xD: return "EV_IPL";
    default: return "other";
    }
}

uint8_t g_soft[24][32];                    // the PCRs as the log says they must be
struct Event { uint32_t pcr, type, size; const uint8_t* digest; const uint8_t* data; };
Event g_ev[64];
uint32_t g_nev;

// Walks a crypto-agile (TPM 2.0) event log. Returns the number of events, 0 if the
// header is not a "Spec ID Event03".
uint32_t replay_log(const uint8_t* log, uint32_t size)
{
    // Event 0 has the old TPM 1.2 layout: pcr, type, SHA-1 digest (20), size, data.
    if (size < 32 || le<uint32_t>(log + 4) != 0x3) return 0;
    const uint32_t first = le<uint32_t>(log + 28);
    const uint8_t* spec = log + 32;
    if (memcmp(spec, "Spec ID Event03", 15) != 0) return 0;
    const uint32_t nalg = le<uint32_t>(spec + 24);
    uint16_t sizes[8] = {}, algs[8] = {};
    kprintf("log: header \"Spec ID Event03\", %u algorithm(s):", nalg);
    for (uint32_t i = 0; i < nalg && i < 8; ++i) {
        algs[i] = le<uint16_t>(spec + 28 + 4 * i);
        sizes[i] = le<uint16_t>(spec + 30 + 4 * i);
        kprintf(" 0x%04x (%u-byte digests)", algs[i], sizes[i]);
    }
    kprintf("\n");
    const uint8_t* p = log + 32 + first;
    const uint8_t* end = log + size;
    uint32_t n = 0;
    while (p + 12 <= end) {
        const uint32_t pcr = le<uint32_t>(p), type = le<uint32_t>(p + 4), count = le<uint32_t>(p + 8);
        if (type == 0 && count == 0) break;                 // the zeroed rest of the area
        const uint8_t* q = p + 12;
        const uint8_t* d256 = nullptr;
        for (uint32_t k = 0; k < count; ++k) {
            const uint16_t alg = le<uint16_t>(q);
            uint16_t sz = 0;
            for (uint32_t i = 0; i < nalg && i < 8; ++i) if (algs[i] == alg) sz = sizes[i];
            if (sz == 0) { kprintf("log: unknown algorithm 0x%04x, stop\n", alg); return n; }
            if (alg == tpm2::ALG_SHA256) d256 = q + 2;
            q += 2 + sz;
        }
        const uint32_t esize = le<uint32_t>(q);
        const uint8_t* data = q + 4;
        if (data + esize > end || pcr >= 24) break;
        kprintf("log: pcr %u type 0x%x %s digest ", pcr, type, event_name(type));
        if (d256) hex(d256, 32);
        kprintf(" data ");
        hex(data, esize < 24 ? esize : 24);
        kprintf("%s\n", esize > 24 ? "..." : "");
        if (d256 && type != 0x3) sha::extend(g_soft[pcr], d256);   // EV_NO_ACTION is never extended
        if (d256 && g_nev < 64) g_ev[g_nev++] = Event{pcr, type, esize, d256, data};
        ++n;
        p = data + esize;
    }
    return n;
}

// The "kernel command line" this kernel measures before using it.
const char g_cmdline[64] = "console=ttyS0 root=/dev/vda1 lockdown=integrity";
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-13 kernel: magic=0x%x\n", magic);
    intr::init();

    // 1. The ACPI TPM2 table.
    const auto* t = reinterpret_cast<const uint8_t*>(acpi::find("TPM2"));
    if (!t) {
        kprintf("acpi: no TPM2 table\n");
        tpm::probe();
        kprintf("F4-13 done (no TPM on this machine)\n");
        qemu_exit(0x10);
    }
    const uint32_t len = le<uint32_t>(t + 4);
    const uint64_t control = le<uint64_t>(t + 40);
    const uint32_t start = le<uint32_t>(t + 48);
    const uint32_t laml = len >= 76 ? le<uint32_t>(t + 64) : 0;
    const uint64_t lasa = len >= 76 ? le<uint64_t>(t + 68) : 0;
    kprintf("acpi: TPM2 table revision %u, %u bytes: platform class %u, control area 0x%lx, start method %u (%s), "
            "log area %u bytes at 0x%lx\n", t[8], len, le<uint16_t>(t + 36), control, start,
            start == 6 ? "TIS / FIFO" : start == 7 ? "CRB" : "other", laml, lasa);

    // 2. The TIS interface.
    if (!tpm::probe()) { kprintf("F4-13 FAILED\n"); qemu_exit(1); }

    // 3. The firmware's event log, replayed and compared.
    const uint32_t events = laml ? replay_log(reinterpret_cast<const uint8_t*>(static_cast<uintptr_t>(lasa)), laml) : 0;
    kprintf("log: %u event(s) replayed\n", events);
    bool fw_ok = events > 0;
    for (uint32_t i = 0; i < 8; ++i) {
        uint8_t v[32];
        const uint32_t rc = tpm::pcr_read(i, v);
        const bool same = rc == 0 && memcmp(v, g_soft[i], 32) == 0;
        fw_ok = fw_ok && same;
        kprintf("pcr %u: TPM ", i);
        hex(v, 32);
        kprintf(" %s\n", rc ? "READ FAILED" : same ? "= replay" : "!= replay");
    }
    kprintf("firmware log replay %s the TPM for PCR 0-7\n", fw_ok ? "matches" : "DOES NOT MATCH");

    // 4. Measure the command line into PCR 8, then report.
    uint8_t logged[32], extended[32];
    sha::hash(g_cmdline, kstrlen(g_cmdline), logged);                 // what goes into the log
#ifdef F413_SIZEOF_BUG
    sha::hash(g_cmdline, sizeof g_cmdline, extended);                 // what goes into the TPM
#else
    memcpy(extended, logged, 32);
#endif
    const uint32_t rc = tpm::pcr_extend(8, extended);
    kprintf("measure: \"%s\" (%u bytes) -> PCR 8, TPM2_PCR_Extend response code 0x%x = %s\n", g_cmdline,
            kstrlen(g_cmdline), rc, tpm::rc_name(rc));
    tpm::trace = true;
    uint8_t pcr8[32];
    tpm::pcr_read(8, pcr8);
    tpm::trace = false;
    uint8_t rnd[16];
    const uint32_t rrc = tpm::get_random(rnd, sizeof rnd);
    kprintf("tpm: GetRandom(16) response code 0x%x = %s", rrc, tpm::rc_name(rrc));
    if (rrc == 0) { kprintf(", bytes "); hex(rnd, 16); }
    kprintf("\n");

    // The attestation report: every event of the log plus ours, then the PCR values.
    for (uint32_t i = 0; i < g_nev; ++i) {
        kprintf("report: event pcr %u type 0x%x digest ", g_ev[i].pcr, g_ev[i].type);
        hex(g_ev[i].digest, 32);
        kprintf(" data ");
        hex(g_ev[i].data, g_ev[i].size < 256 ? g_ev[i].size : 256);
        kprintf("\n");
    }
    kprintf("report: event pcr 8 type 0xd digest ");
    hex(logged, 32);
    kprintf(" data ");
    hex(reinterpret_cast<const uint8_t*>(g_cmdline), kstrlen(g_cmdline));
    kprintf("\n");
    for (uint32_t i = 0; i <= 8; ++i) {
        uint8_t v[32];
        tpm::pcr_read(i, v);
        kprintf("report: pcr %u ", i);
        hex(v, 32);
        kprintf("\n");
    }
    kprintf("F4-13 %s\n", fw_ok && rc == 0 ? "done" : "FAILED");
    qemu_exit(fw_ok && rc == 0 ? 0x10 : 1);
}
