// ota.h - F3-41 Listing 1: a host model of firmware updates on a microcontroller's flash.
//   * Flash: erase and page-program operations; a power cut can stop any operation halfway.
//   * Images: header + payload + Ed25519 signature (OpenSSL 3 EVP API, <openssl/evp.h>).
//   * Two designs: SingleSlot (erase and rewrite the only copy; CRC check) and ABSlots
//     (write the other slot, test-boot it once, keep it only if the application confirms).
// The A/B design follows the ideas of MCUboot's "test and confirm" flow but is the
// university's own simplified format (see the chapter's unverified box).
#pragma once
#include <openssl/evp.h>

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using Bytes = std::vector<uint8_t>;

// ---------------------------------------------------------------- flash with power cuts --
class Flash {
public:
    static constexpr size_t kPage = 256;     // smallest programmable unit in this model
    static constexpr size_t kSector = 4096;  // smallest erasable unit in this model
    explicit Flash(size_t size) : mem_(size, 0xFF) {}

    long ops = 0;          // operations started so far
    long cutAt = -1;       // power fails during operation number cutAt (-1 = never)
    bool powered = true;

    bool erase(size_t addr)                   // erase one sector
    {
        if (!begin()) { return false; }
        const size_t n = powered ? kSector : kSector / 2;   // cut: only half erased
        std::fill(mem_.begin() + static_cast<long>(addr), mem_.begin() + static_cast<long>(addr + n), 0xFF);
        return powered;
    }

    bool program(size_t addr, const uint8_t* data, size_t n)   // n <= one page
    {
        if (!begin()) { return false; }
        const size_t done = powered ? n : n / 2;             // cut: only half written
        for (size_t i = 0; i < done; ++i) { mem_[addr + i] &= data[i]; }   // bits go 1 -> 0
        return powered;
    }

    const uint8_t* at(size_t addr) const { return mem_.data() + addr; }
    void restorePower() { powered = true; cutAt = -1; }

private:
    bool begin()
    {
        if (!powered) { return false; }
        if (ops++ == cutAt) { powered = false; }             // this one is interrupted
        return true;
    }
    Bytes mem_;
};

// ------------------------------------------------------------------ signing (Ed25519) --
struct PkeyFree { void operator()(EVP_PKEY* p) const { EVP_PKEY_free(p); } };
struct MdCtxFree { void operator()(EVP_MD_CTX* c) const { EVP_MD_CTX_free(c); } };
using Pkey = std::unique_ptr<EVP_PKEY, PkeyFree>;

// A fixed 32-byte seed makes the key, and therefore every signature, repeatable.
inline Pkey keyFromSeed(const char* label)
{
    uint8_t seed[32] = {};
    for (size_t i = 0; i < 32 && label[i] != '\0'; ++i) { seed[i] = static_cast<uint8_t>(label[i]); }
    return Pkey(EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, seed, sizeof seed));
}

inline Pkey publicPart(const Pkey& k)
{
    uint8_t pub[32];
    size_t n = sizeof pub;
    EVP_PKEY_get_raw_public_key(k.get(), pub, &n);
    return Pkey(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, pub, n));
}

inline Bytes sign(const Pkey& k, const Bytes& msg)
{
    std::unique_ptr<EVP_MD_CTX, MdCtxFree> ctx(EVP_MD_CTX_new());
    Bytes sig(64);
    size_t n = sig.size();
    if (EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, k.get()) != 1 ||
        EVP_DigestSign(ctx.get(), sig.data(), &n, msg.data(), msg.size()) != 1) {
        sig.clear();
    }
    return sig;
}

inline bool verify(const Pkey& pub, const uint8_t* msg, size_t n, const uint8_t* sig)
{
    std::unique_ptr<EVP_MD_CTX, MdCtxFree> ctx(EVP_MD_CTX_new());
    return EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, pub.get()) == 1 &&
           EVP_DigestVerify(ctx.get(), sig, 64, msg, n) == 1;
}

// ------------------------------------------------------------------------- images --
// Layout: magic(4) version(4) size(4) crc32(4) | payload(size) | signature(64)
constexpr uint32_t kMagic = 0x3530334F;
constexpr size_t kHeader = 16;

inline uint32_t crc32(const uint8_t* p, size_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; ++i) {
        c ^= p[i];
        for (int k = 0; k < 8; ++k) { c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u))); }
    }
    return ~c;
}

inline void put32(Bytes& b, uint32_t v) { for (int i = 0; i < 4; ++i) { b.push_back(static_cast<uint8_t>(v >> (8 * i))); } }
inline uint32_t get32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t{p[3]} << 24); }

inline Bytes makeImage(uint32_t version, size_t size, const Pkey& key)
{
    Bytes payload(size);
    for (size_t i = 0; i < size; ++i) { payload[i] = static_cast<uint8_t>(version * 31 + i * 7); }
    Bytes img;
    put32(img, kMagic); put32(img, version); put32(img, static_cast<uint32_t>(size));
    put32(img, crc32(payload.data(), payload.size()));
    img.insert(img.end(), payload.begin(), payload.end());
    const Bytes sig = sign(key, img);                      // signs header + payload
    img.insert(img.end(), sig.begin(), sig.end());
    return img;
}

struct ImageInfo { bool present = false, crcOk = false, sigOk = false; uint32_t version = 0; };

inline ImageInfo inspect(const Flash& f, size_t base, size_t slotSize, const Pkey* trusted)
{
    ImageInfo r;
    const uint8_t* p = f.at(base);
    if (get32(p) != kMagic) { return r; }
    r.present = true;
    r.version = get32(p + 4);
    const uint32_t size = get32(p + 8);
    if (kHeader + size + 64 > slotSize) { return r; }
    r.crcOk = crc32(p + kHeader, size) == get32(p + 12);
    r.sigOk = trusted != nullptr && verify(*trusted, p, kHeader + size, p + kHeader + size);
    return r;
}

inline bool writeImage(Flash& f, size_t base, const Bytes& img)
{
    for (size_t s = 0; s < img.size(); s += Flash::kSector) {
        if (!f.erase(base + s)) { return false; }
    }
    for (size_t off = 0; off < img.size(); off += Flash::kPage) {
        const size_t n = std::min(Flash::kPage, img.size() - off);
        if (!f.program(base + off, img.data() + off, n)) { return false; }
    }
    return true;
}

// ---------------------------------------------------- design 1: a single slot, CRC only --
struct SingleSlot {
    static constexpr size_t kSlot = 0, kSlotSize = 32768;
    Flash flash{40960};
    std::string log;

    void factory(const Bytes& img) { writeImage(flash, kSlot, img); }
    bool update(const Bytes& img) { log += "erase+write slot; "; return writeImage(flash, kSlot, img); }
    int boot()                                             // returns the version, -1 = bricked
    {
        const ImageInfo i = inspect(flash, kSlot, kSlotSize, nullptr);
        if (i.present && i.crcOk) { log += "boot v" + std::to_string(i.version) + "; "; return static_cast<int>(i.version); }
        log += "boot: NO VALID IMAGE (bricked); ";
        return -1;
    }
    void confirm() {}
};

// ------------------------------------- design 2: A/B slots, signatures, test and confirm --
struct ABSlots {
    static constexpr size_t kSlotSize = 16384;
    static constexpr size_t kSlotBase[2] = {0, 16384};
    static constexpr size_t kStatePage[2] = {32768, 36864};   // two copies, in two sectors
    Flash flash{40960};
    Pkey trusted;                                          // the public key in the boot loader
    std::string log;
    int running = -1;                                      // slot running now

    struct State { uint32_t seq = 0, active = 0, pending = 0, tried = 0, minVersion = 0; };

    explicit ABSlots(const Pkey& signer) : trusted(publicPart(signer)) {}

    State readState() const
    {
        State best;
        bool found = false;
        for (size_t k = 0; k < 2; ++k) {
            const uint8_t* p = flash.at(kStatePage[k]);
            if (get32(p) != kMagic || crc32(p, 24) != get32(p + 24)) { continue; }   // torn or empty
            State s{get32(p + 4), get32(p + 8), get32(p + 12), get32(p + 16), get32(p + 20)};
            if (!found || s.seq > best.seq) { best = s; found = true; }
        }
        return best;
    }

    bool writeState(State s)                               // never overwrite the newest copy
    {
        const State cur = readState();
        s.seq = cur.seq + 1;
        const size_t page = kStatePage[s.seq % 2];
        Bytes b;
        put32(b, kMagic); put32(b, s.seq); put32(b, s.active); put32(b, s.pending);
        put32(b, s.tried); put32(b, s.minVersion); put32(b, crc32(b.data(), 24));
        return flash.erase(page) && flash.program(page, b.data(), b.size());
    }

    void factory(const Bytes& img)
    {
        writeImage(flash, kSlotBase[0], img);
        writeState(State{0, 0, 0, 0, 1});
    }

    bool update(const Bytes& img)                          // runs inside the application
    {
        State s = readState();
        const uint32_t target = 1 - s.active;
        log += "write slot " + std::string(target ? "B" : "A") + "; ";
        if (!writeImage(flash, kSlotBase[target], img)) { return false; }
        s.pending = 1; s.tried = 0;
        log += "mark pending; ";
        return writeState(s);
    }

    bool acceptable(uint32_t slot, const State& s, uint32_t& version) const
    {
        const ImageInfo i = inspect(flash, kSlotBase[slot], kSlotSize, &trusted);
        version = i.version;
        return i.present && i.crcOk && i.sigOk && i.version >= s.minVersion;
    }

    int boot()
    {
        State s = readState();
        uint32_t v = 0;
        if (s.pending != 0) {
            const uint32_t other = 1 - s.active;
            if (s.tried != 0) {                            // trial boot did not confirm
                log += "revert (new image never confirmed); ";
                s.pending = 0; s.tried = 0; writeState(s);
            } else if (acceptable(other, s, v)) {
                s.tried = 1; writeState(s);
                log += "trial boot v" + std::to_string(v) + "; ";
                running = static_cast<int>(other);
                return static_cast<int>(v);
            } else {
                log += "REFUSED new image (signature, damage or downgrade); ";
                s.pending = 0; writeState(s);
            }
        }
        for (uint32_t k = 0; k < 2; ++k) {                 // active slot first, then the other
            const uint32_t slot = k == 0 ? s.active : 1 - s.active;
            if (acceptable(slot, s, v)) {
                log += "boot v" + std::to_string(v) + "; ";
                running = static_cast<int>(slot);
                return static_cast<int>(v);
            }
        }
        log += "boot: NO VALID IMAGE (bricked); ";
        return -1;
    }

    void confirm()                                         // the application is healthy
    {
        State s = readState();
        if (s.pending != 0 && running == static_cast<int>(1 - s.active)) {
            uint32_t v = 0;
            acceptable(static_cast<uint32_t>(running), s, v);
            s.active = static_cast<uint32_t>(running); s.pending = 0; s.tried = 0; s.minVersion = v;
            writeState(s);
            log += "confirm v" + std::to_string(v) + "; ";
        }
    }
};
