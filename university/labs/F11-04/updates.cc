// updates.cc - F11-04 Listing 2: a device with two flash slots receives signed updates.
// The boot loader validates the candidate (fwimage.h), swaps the slots sector by sector with
// a status record so that a power cut can be resumed, test-boots the new image, and raises the
// device's security counter only when the new image confirms itself.
//
//   updates demo        the H4 acceptance tests and the other scenarios
//   updates forensic    the evidence pack of the forensic lab
#include <algorithm>
#include <cstring>
#include <iostream>

#include "fwimage.h"

namespace {

using fw::Bytes;
constexpr size_t kSector = 128;
constexpr int kSectors = 4;

Bytes to_slot(const Bytes& image)
{
    Bytes slot(kSector * kSectors, 0xff);   // erased flash reads as 0xff
    std::copy(image.begin(), image.end(), slot.begin());
    return slot;
}

// The image inside a slot: its length is found from the header and the TLV area's size field.
Bytes from_slot(const Bytes& slot)
{
    if (fw::get32(slot, 0) != fw::kMagic) { return {}; }
    const size_t end = fw::kHeaderSize + fw::get32(slot, 8);
    if (end + 4 > slot.size()) { return {}; }
    const size_t total = fw::get16(slot, end + 2);
    if (end + total > slot.size()) { return {}; }
    return Bytes(slot.begin(), slot.begin() + long(end + total));
}

std::string name_of(const Bytes& slot)
{
    const Bytes img = from_slot(slot);
    const auto p = fw::parse(img);
    if (!p) { return "(no valid image)"; }
    return std::string(p->signed_region.begin() + fw::kHeaderSize, p->signed_region.end());
}

struct Device {
    Bytes primary, secondary, scratch = Bytes(kSector, 0xff);
    // status record (in flash): which phase, how far the swap got
    enum Phase { kIdle, kSwapping, kTesting, kReverting } phase = kIdle;
    int step = 0;                 // operations of the current swap already done
    bool confirmed = true;
    uint32_t counter = 1;         // security counter in one-time-programmable memory
    std::vector<EVP_PKEY*> trusted;
    long budget = -1;             // operations left before a simulated power cut (-1 = no cut)

    bool spend()                  // one flash write; false when the power fails before it
    {
        if (budget == 0) { return false; }
        if (budget > 0) { --budget; }
        return true;
    }

    // One swap = for each sector: primary->scratch, secondary->primary, scratch->secondary,
    // each write followed by a status update. Every write can be repeated safely after a cut.
    bool swap()
    {
        for (int op = step; op < 3 * kSectors; ++op) {
            const size_t s = size_t(op / 3) * kSector;
            if (!spend()) { return false; }
            switch (op % 3) {
            case 0: std::copy(primary.begin() + long(s), primary.begin() + long(s + kSector), scratch.begin()); break;
            case 1: std::copy(secondary.begin() + long(s), secondary.begin() + long(s + kSector), primary.begin() + long(s)); break;
            case 2: std::copy(scratch.begin(), scratch.end(), secondary.begin() + long(s)); break;
            }
            if (!spend()) { return false; }
            step = op + 1;
        }
        return true;
    }

    // One power-on. Returns what ran, or why nothing did.
    std::string boot(bool new_image_confirms = true)
    {
        if (phase == kSwapping || phase == kReverting) {          // resume after a power cut
            if (!swap()) { return "power lost during swap"; }
            if (!spend()) { return "power lost"; }
            step = 0;
            phase = (phase == kSwapping) ? kTesting : kIdle;
            confirmed = phase == kIdle;
        } else if (phase == kTesting && !confirmed) {             // the last test boot failed
            if (!spend()) { return "power lost"; }
            phase = kReverting;
            step = 0;
            if (!swap()) { return "power lost during revert"; }
            if (!spend()) { return "power lost"; }
            phase = kIdle;
            confirmed = true;
            secondary.assign(secondary.size(), 0xff);              // the failed image is not retried
        } else if (phase == kIdle && from_slot(secondary).size() != 0) {   // an update is waiting
            const fw::Verdict v = fw::validate(from_slot(secondary), trusted, counter);
            if (!v.ok) {
                secondary.assign(secondary.size(), 0xff);          // erase the rejected candidate
                return "update REFUSED (" + v.reason + "); running " + run_primary();
            }
            if (!spend()) { return "power lost"; }
            phase = kSwapping;
            step = 0;
            if (!swap()) { return "power lost during swap"; }
            if (!spend()) { return "power lost"; }
            phase = kTesting;
            confirmed = false;
        }
        if (phase == kTesting && !confirmed) {
            if (!new_image_confirms) { return "test boot of " + name_of(primary) + ": crashed before confirming"; }
            confirmed = true;                                      // the new image confirms itself
            phase = kIdle;
            const auto p = fw::parse(from_slot(primary));
            counter = std::max(counter, p->header.security_counter);   // only now raise the counter
            secondary.assign(secondary.size(), 0xff);
        }
        return run_primary();
    }

    // The primary is validated on every boot too (its counter was accepted when it was installed).
    std::string run_primary()
    {
        const fw::Verdict v = fw::validate(from_slot(primary), trusted, 0);
        return v.ok ? name_of(primary) : "NOTHING (primary invalid: " + v.reason + ")";
    }
};

int demo()
{
    fw::Pkey a = fw::load_or_make_key("vendor_a"), b = fw::load_or_make_key("attacker_b"),
             c = fw::load_or_make_key("vendor_c_offline");
    std::cout << "keys: vendor A " << fw::hex(fw::key_hash(a.get()).data(), 4) << "..., attacker B "
              << fw::hex(fw::key_hash(b.get()).data(), 4) << "..., vendor C (offline) "
              << fw::hex(fw::key_hash(c.get()).data(), 4) << "...\n";
    const Bytes v10 = fw::build_image("app 1.0", 1, 0, 1, a.get());
    const Bytes v11 = fw::build_image("app 1.1", 1, 1, 1, a.get());
    std::cout << "image app 1.1: " << v11.size() << " bytes (header 32, payload 7, TLV area "
              << v11.size() - 39 << ")\n";

    auto fresh = [&]() {
        Device d;
        d.primary = to_slot(v10);
        d.secondary = to_slot({});
        d.trusted = {a.get(), c.get()};
        return d;
    };

    std::cout << "\n1. Normal update 1.0 -> 1.1\n";
    Device d = fresh();
    d.secondary = to_slot(v11);
    std::cout << "    boot 1: " << d.boot() << "\n    boot 2: " << d.boot() << "\n";

    std::cout << "\n2. H4: updates with a wrong signature are refused\n";
    struct Bad { std::string what; Bytes img; };
    std::vector<Bad> bad;
    Bytes flipped = v11;
    flipped[fw::kHeaderSize + 4] ^= 0x01;                               // one payload bit
    bad.push_back({"payload changed after signing", flipped});
    Bytes newer = v11;
    newer[13] = 9;                                                      // header: version 1.9
    bad.push_back({"version field edited after signing", newer});
    bad.push_back({"signed with the attacker's key B", fw::build_image("app 1.1-evil", 1, 1, 1, b.get())});
    Bytes forged = fw::build_image("app 1.1-evil", 1, 1, 1, b.get());
    const auto ka = fw::key_hash(a.get());
    std::copy(ka.begin(), ka.end(), forged.begin() + long(fw::kHeaderSize + 12 + 4 + 4 + 32 + 4));
    bad.push_back({"signed with B, key-hash record changed to A's", forged});
    for (const Bad& x : bad) {
        Device e = fresh();
        e.secondary = to_slot(x.img);
        std::cout << "    " << x.what << ":\n      " << e.boot() << "\n";
    }

    std::cout << "\n3. Rollback: 2.0 (counter 2) is installed, then the genuine old 1.1 (counter 1) is offered\n";
    Device r = fresh();
    r.secondary = to_slot(fw::build_image("app 2.0", 2, 0, 2, a.get()));
    std::cout << "    " << r.boot() << ", device counter now " << r.counter << "\n";
    r.secondary = to_slot(v11);
    std::cout << "    " << r.boot() << "\n";

    std::cout << "\n4. H4: power loss during the update (cut before each of the flash writes in turn)\n";
    int total = 0, old_runs = 0, new_runs = 0, bricked = 0;
    for (long cut = 0;; ++cut) {
        Device p = fresh();
        p.secondary = to_slot(v11);
        p.budget = cut;
        const std::string first = p.boot();
        if (first.find("power lost") == std::string::npos) { break; }   // no cut happened: sweep done
        ++total;
        p.budget = -1;                                                   // power is back
        const std::string after = p.boot();
        if (after == "app 1.1") { ++new_runs; } else if (after == "app 1.0") { ++old_runs; } else { ++bricked; }
        if (cut == 7) { std::cout << "    example, cut before write 8: \"" << first << "\", next boot: " << after << "\n"; }
    }
    std::cout << "    " << total << " cut points: " << new_runs << " finished the update on the next boot and ran 1.1, "
              << old_runs << " ran 1.0, " << bricked << " ran nothing\n";
    {
        Device p = fresh();
        p.budget = 0;
        p.secondary = to_slot(v11);
        std::cout << "    cut before the first write: " << p.boot();
        p.budget = -1;
        std::cout << "; next boot: " << p.boot() << "\n";
    }

    std::cout << "\n5. The new image crashes before confirming itself\n";
    Device t = fresh();
    t.secondary = to_slot(fw::build_image("app 2.0-broken", 2, 0, 2, a.get()));
    std::cout << "    boot 1: " << t.boot(false) << "\n    boot 2: " << t.boot() << ", device counter "
              << t.counter << "\n";

    std::cout << "\n6. Key rotation planned at manufacture: an image signed with the offline key C\n";
    Device k = fresh();
    k.secondary = to_slot(fw::build_image("app 3.0 (signed with C)", 3, 0, 3, c.get()));
    std::cout << "    " << k.boot() << "\n";
    return 0;
}

int forensic()
{
    fw::Pkey a = fw::load_or_make_key("vendor_a");
    const Bytes v12 = fw::build_image("app 1.2 (has the remote-unlock bug)", 1, 2, 2, a.get());
    Bytes edited = v12;
    // the attacker edits only the unprotected counter copy at the end of the image
    fw::put32(edited, edited.size() - 4, 9);
    std::cout << "EVIDENCE 1 - the image found on the robot vs the vendor's release of 1.2 (last 24 bytes)\n";
    std::cout << "    vendor 1.2: ..." << fw::hex(v12.data() + v12.size() - 24, 24) << "\n";
    std::cout << "    on robot:   ..." << fw::hex(edited.data() + edited.size() - 24, 24) << "\n";
    std::cout << "    sizes " << v12.size() << " and " << edited.size() << " bytes; SHA-256 of header+payload: "
              << fw::hex(fw::sha256(fw::parse(v12)->signed_region).data(), 6) << "... and "
              << fw::hex(fw::sha256(fw::parse(edited)->signed_region).data(), 6) << "...\n";
    std::cout << "EVIDENCE 2 - boot loader log on the robot (device counter 5, running 1.5)\n";
    const fw::Verdict buggy = fw::validate(edited, {a.get()}, 5, true);
    std::cout << "    candidate: " << buggy.reason << " -> " << (buggy.ok ? "INSTALLED" : "refused") << "\n";
    std::cout << "EVIDENCE 3 - the same candidate checked with the release-test boot loader\n";
    const fw::Verdict good = fw::validate(edited, {a.get()}, 5, false);
    std::cout << "    candidate: " << good.reason << " -> " << (good.ok ? "INSTALLED" : "refused") << "\n";
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
