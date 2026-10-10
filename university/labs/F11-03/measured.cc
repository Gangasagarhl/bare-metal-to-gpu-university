// measured.cc - F11-03 Listing 2: measured boot and remote attestation with the model of
// tpm_model.h. A simulated firmware measures what it starts into PCRs and an event log; a
// remote verifier sends a nonce, receives a signed quote and the log, replays the log and
// judges the boot against its list of known-good values. Sealing shows the other use of PCRs.
//
//   measured demo       six scenarios
//   measured forensic   the evidence pack of the forensic lab
#include <cstring>
#include <map>

#include "tpm_model.h"

namespace {

// What a machine boots: each string stands for the bytes of a component.
struct Platform {
    std::string firmware = "firmware build 2026.04";
    std::string boot_order = "Boot0001 = disk";
    bool secure_boot = true;
    std::string db = "db: SS301 lab signing key 2026";
    std::string dbx = "dbx: (empty)";
    std::vector<std::string> boot_apps = {"loader v3 (signed by db1)"};
    std::string kernel = "kernel 1.8";
};

// The firmware's measurements, in boot order. PCR use follows this model's assignment:
// 0 firmware code, 1 firmware configuration, 4 boot applications, 7 Secure Boot policy,
// 8 the kernel (measured by the loader).
std::vector<Event> boot(Tpm& tpm, const Platform& p)
{
    std::vector<Event> log;
    auto measure = [&](int pcr, const std::string& type, const std::string& what) {
        const Event e{pcr, type, sha256(what), what};
        tpm.pcrs().extend(e.pcr, e.digest);
        log.push_back(e);
    };
    measure(0, "firmware", p.firmware);
    measure(7, "sb-variable", p.secure_boot ? "SecureBoot = 1" : "SecureBoot = 0");
    measure(7, "sb-variable", p.db);
    measure(7, "sb-variable", p.dbx);
    measure(1, "config", p.boot_order);
    for (int pcr : {0, 1, 4, 7}) {
        measure(pcr, "separator", "end of firmware measurements");
    }
    for (const std::string& app : p.boot_apps) {
        measure(4, "boot-app", app);
    }
    measure(8, "kernel", p.kernel);
    return log;
}

// The verifier's knowledge: which component digests it accepts, and its policy.
struct Verifier {
    std::map<std::string, std::string> known;   // digest (hex, 64 digits) -> name
    void allow(const std::string& what) { known[hex(sha256(what), 32)] = what; }

    bool judge(EVP_PKEY* ak, const Bytes& nonce_sent, const Quote& q, const std::vector<Event>& log)
    {
        bool ok = true;
        std::cout << "    signature by the TPM's attestation key: "
                  << (verify_signature(ak, q) ? "valid" : "INVALID") << "\n";
        ok = ok && verify_signature(ak, q);
        const bool fresh = q.nonce == nonce_sent;
        std::cout << "    nonce: " << (fresh ? "the one we sent" : "NOT the one we sent (old quote?)") << "\n";
        ok = ok && fresh;
        const Digest replayed = composite(replay(log), q.selection);
        std::cout << "    PCR digest in quote " << hex(q.pcr_digest) << ", from replaying the log "
                  << hex(replayed) << (replayed == q.pcr_digest ? "  equal" : "  DIFFERENT: the log is not what was measured") << "\n";
        ok = ok && replayed == q.pcr_digest;
        for (const Event& e : log) {
            if (e.type != "separator" && known.count(hex(e.digest, 32)) == 0) {
                std::cout << "    unknown component in PCR " << e.pcr << ": " << hex(e.digest) << " \"" << e.description << "\"\n";
                ok = false;
            }
        }
        std::cout << "    verdict: " << (ok ? "TRUSTED" : "NOT TRUSTED") << "\n";
        return ok;
    }
};

void print_log(const std::vector<Event>& log)
{
    std::cout << "    #   PCR  type         digest    description\n";
    for (size_t i = 0; i < log.size(); ++i) {
        const Event& e = log[i];
        std::cout << "    " << (i < 10 ? " " : "") << i << "  " << e.pcr << "    " << e.type
                  << std::string(13 - e.type.size(), ' ') << hex(e.digest) << "  " << e.description << "\n";
    }
}

void print_pcrs(const PcrBank& bank, const std::vector<int>& which)
{
    for (int p : which) {
        std::cout << "    PCR " << p << " = " << hex(bank.read(p), 8) << "...\n";
    }
}

Verifier make_verifier()
{
    Verifier v;
    const Platform good;
    for (const std::string& s : {good.firmware, good.boot_order, good.db, good.dbx, good.kernel,
                                 std::string("SecureBoot = 1")}) {
        v.allow(s);
    }
    v.allow("loader v3 (signed by db1)");
    return v;
}

const std::vector<int> kSelection = {0, 1, 4, 7, 8};

int demo()
{
    Verifier verifier = make_verifier();
    const Bytes nonce1 = {0x4e, 0x31, 0x9a, 0x07};

    std::cout << "1. Healthy boot\n";
    Tpm tpm;
    std::vector<Event> log = boot(tpm, Platform{});
    print_log(log);
    print_pcrs(tpm.pcrs(), kSelection);
    const Quote q1 = tpm.quote(nonce1, kSelection);
    verifier.judge(tpm.attestation_key(), nonce1, q1, log);
    const Tpm::Sealed disk_key = tpm.seal("disk key 7f3a", {7});
    std::string secret;
    std::cout << "    disk key sealed to PCR 7; unseal now: " << (tpm.unseal(disk_key, secret) ? "released \"" + secret + "\"" : "REFUSED") << "\n";

    std::cout << "2. Next boot: Secure Boot switched off and another loader started (PCRs start from zero again)\n";
    Tpm tpm2;
    Platform evil;
    evil.secure_boot = false;
    evil.boot_apps = {"loader v3 patched to skip the kernel check"};
    std::vector<Event> log2 = boot(tpm2, evil);
    print_pcrs(tpm2.pcrs(), {4, 7});
    const Bytes nonce2 = {0x11, 0x22, 0x33, 0x44};
    verifier.judge(tpm2.attestation_key(), nonce2, tpm2.quote(nonce2, kSelection), log2);
    const Tpm::Sealed key_on_2 = tpm2.seal("unused", {7});
    std::cout << "    PCR 7 differs from scenario 1: " << (key_on_2.policy != disk_key.policy ? "yes" : "no")
              << ", so the disk key sealed in scenario 1 would not be released\n";

    std::cout << "3. Same boot as 2, but the operating system edits the log to show the good loader\n";
    std::vector<Event> lying = log2;
    for (Event& e : lying) {
        if (e.description == "SecureBoot = 0") { e.digest = sha256("SecureBoot = 1"); e.description = "SecureBoot = 1"; }
        if (e.type == "boot-app") { e.digest = sha256("loader v3 (signed by db1)"); e.description = "loader v3 (signed by db1)"; }
    }
    const Bytes nonce3 = {0x55, 0x66, 0x77, 0x88};
    verifier.judge(tpm2.attestation_key(), nonce3, tpm2.quote(nonce3, kSelection), lying);

    std::cout << "4. Same boot as 2, but the machine sends the old quote and log from scenario 1\n";
    const Bytes nonce4 = {0x99, 0xaa, 0xbb, 0xcc};
    verifier.judge(tpm.attestation_key(), nonce4, q1, log);

    std::cout << "5. Legitimate key rotation: dbx gains the 2026 key, db the 2027 key, loader re-signed\n";
    Tpm tpm5;
    Platform rotated;
    rotated.db = "db: SS301 lab signing key 2026, SS301 lab signing key 2027";
    rotated.dbx = "dbx: SS301 lab signing key 2026";
    rotated.boot_apps = {"loader v3 (signed by db2)"};
    std::vector<Event> log5 = boot(tpm5, rotated);
    std::string s5;
    // the same sealed object as scenario 1, presented to a TPM whose PCR 7 now holds the new policy
    std::cout << "    unseal the disk key sealed before the rotation: "
              << (tpm5.unseal(disk_key, s5) ? "released" : "REFUSED (PCR 7 changed): recovery key needed") << "\n";
    const Bytes nonce5 = {0x01, 0x02, 0x03, 0x05};
    verifier.judge(tpm5.attestation_key(), nonce5, tpm5.quote(nonce5, kSelection), log5);
    std::cout << "    after the verifier learns the new values and the key is sealed again:\n";
    for (const std::string& s : {rotated.db, rotated.dbx, rotated.boot_apps[0]}) {
        verifier.allow(s);
    }
    verifier.judge(tpm5.attestation_key(), nonce5, tpm5.quote(nonce5, kSelection), log5);
    const Tpm::Sealed resealed = tpm5.seal("disk key 7f3a", {7});
    std::cout << "    unseal the resealed key: " << (tpm5.unseal(resealed, s5) ? "released" : "REFUSED") << "\n";

    std::cout << "6. Order matters: the same two digests extended in both orders into an empty PCR\n";
    PcrBank a, b;
    a.extend(0, sha256("A"));
    a.extend(0, sha256("B"));
    b.extend(0, sha256("B"));
    b.extend(0, sha256("A"));
    std::cout << "    A then B: " << hex(a.read(0), 8) << "...  B then A: " << hex(b.read(0), 8) << "...\n";
    return 0;
}

int forensic()
{
    Verifier verifier = make_verifier();
    Tpm tpm;
    Platform p;
    p.boot_order = "Boot0003 = USB, Boot0001 = disk";
    p.boot_apps = {"UEFI shell 2.2 from USB", "loader v3 (signed by db1)"};
    std::vector<Event> log = boot(tpm, p);
    const Bytes nonce = {0xd0, 0x0d, 0x2b, 0x10};
    std::cout << "EVIDENCE 1 - event log sent by robot R-2207 with its quote\n";
    print_log(log);
    std::cout << "EVIDENCE 2 - the attestation server's report\n";
    verifier.judge(tpm.attestation_key(), nonce, tpm.quote(nonce, kSelection), log);
    std::cout << "EVIDENCE 3 - PCR values of the same robot model in the golden record vs R-2207\n";
    Tpm golden;
    boot(golden, Platform{});
    for (int pcr : kSelection) {
        std::cout << "    PCR " << pcr << "  golden " << hex(golden.pcrs().read(pcr)) << "  R-2207 "
                  << hex(tpm.pcrs().read(pcr)) << (golden.pcrs().read(pcr) == tpm.pcrs().read(pcr) ? "  same" : "  DIFFERENT") << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "forensic") == 0) {
        return forensic();
    }
    return demo();
}
