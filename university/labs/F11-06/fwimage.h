// fwimage.h - (copied unchanged in its code from the F11-04 lab, where it is Listing 1) a signed
// firmware image format and its validation.
// The university's own format, built on the same ideas as MCUboot's (header, payload, a
// trailer of type-length-value records holding a hash, a key hash and a signature); it is NOT
// MCUboot's byte layout. Signatures are ECDSA P-256 with SHA-256 through OpenSSL's EVP API.
//
//   header (32 bytes, little-endian) | payload | TLV area: magic, total size, then records
//   signed region = header + payload (so version and security counter are covered)
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

#include "sha256.h"

namespace fw {

using Bytes = std::vector<uint8_t>;
using Digest = std::array<uint8_t, 32>;

constexpr uint32_t kMagic = 0x31305353;       // "SS01" in memory order
constexpr uint16_t kHeaderSize = 32;
constexpr uint16_t kTlvMagic = 0x5454;        // "TT"
enum TlvType : uint16_t { kTlvSha256 = 0x10, kTlvKeyHash = 0x01, kTlvSignature = 0x20, kTlvInfoCounter = 0x50 };

struct Header {
    uint32_t magic = kMagic;
    uint16_t header_size = kHeaderSize;
    uint32_t payload_size = 0;
    uint8_t major = 0, minor = 0;
    uint16_t revision = 0;
    uint32_t security_counter = 0;   // anti-rollback: the device refuses smaller values
};

inline Digest sha256(const Bytes& b)
{
    Sha256 h;
    h.update(b.data(), b.size());
    Digest d{};
    h.finish(d.data());
    return d;
}

inline std::string hex(const uint8_t* p, size_t n)
{
    std::string s;
    for (size_t i = 0; i < n; ++i) {
        s += "0123456789abcdef"[p[i] >> 4];
        s += "0123456789abcdef"[p[i] & 0xf];
    }
    return s;
}

inline void put16(Bytes& b, size_t at, uint16_t v) { b[at] = uint8_t(v); b[at + 1] = uint8_t(v >> 8); }
inline void put32(Bytes& b, size_t at, uint32_t v) { for (int i = 0; i < 4; ++i) { b[at + i] = uint8_t(v >> (8 * i)); } }
inline uint16_t get16(const Bytes& b, size_t at) { return uint16_t(b[at] | (b[at + 1] << 8)); }
inline uint32_t get32(const Bytes& b, size_t at)
{
    return uint32_t(b[at]) | (uint32_t(b[at + 1]) << 8) | (uint32_t(b[at + 2]) << 16) | (uint32_t(b[at + 3]) << 24);
}

inline Bytes encode_header(const Header& h)
{
    Bytes b(kHeaderSize, 0);
    put32(b, 0, h.magic);
    put16(b, 4, h.header_size);
    put32(b, 8, h.payload_size);
    b[12] = h.major;
    b[13] = h.minor;
    put16(b, 14, h.revision);
    put32(b, 16, h.security_counter);
    return b;
}

// ---- keys (RAII wrappers around OpenSSL objects) ----
struct PkeyFree { void operator()(EVP_PKEY* k) const { EVP_PKEY_free(k); } };
struct MdCtxFree { void operator()(EVP_MD_CTX* c) const { EVP_MD_CTX_free(c); } };
using Pkey = std::unique_ptr<EVP_PKEY, PkeyFree>;
using MdCtx = std::unique_ptr<EVP_MD_CTX, MdCtxFree>;

// Loads keys/<name>.pem, creating a new P-256 key the first time (kept, so key hashes stay stable).
inline Pkey load_or_make_key(const std::string& name)
{
    const std::string path = "keys/" + name + ".pem";
    if (FILE* f = std::fopen(path.c_str(), "r")) {
        Pkey k(PEM_read_PrivateKey(f, nullptr, nullptr, nullptr));
        std::fclose(f);
        if (k) { return k; }
    }
    Pkey k(EVP_EC_gen("P-256"));
    FILE* f = std::fopen(path.c_str(), "w");
    if (!k || f == nullptr || PEM_write_PrivateKey(f, k.get(), nullptr, nullptr, 0, nullptr, nullptr) != 1) {
        throw std::runtime_error("cannot create " + path);
    }
    std::fclose(f);
    return k;
}

// The key hash identifies a public key: SHA-256 of its DER SubjectPublicKeyInfo encoding.
inline Digest key_hash(EVP_PKEY* key)
{
    unsigned char* der = nullptr;
    const int n = i2d_PUBKEY(key, &der);
    if (n <= 0) { throw std::runtime_error("i2d_PUBKEY failed"); }
    Bytes b(der, der + n);
    OPENSSL_free(der);
    return sha256(b);
}

inline void add_tlv(Bytes& area, uint16_t type, const Bytes& value)
{
    Bytes rec(4, 0);
    put16(rec, 0, type);
    put16(rec, 2, uint16_t(value.size()));
    area.insert(area.end(), rec.begin(), rec.end());
    area.insert(area.end(), value.begin(), value.end());
}

// ---- the build server's side ----
inline Bytes build_image(const std::string& payload_text, uint8_t major, uint8_t minor, uint32_t counter,
                         EVP_PKEY* signing_key)
{
    Header h;
    h.payload_size = uint32_t(payload_text.size());
    h.major = major;
    h.minor = minor;
    h.security_counter = counter;
    Bytes signed_region = encode_header(h);
    signed_region.insert(signed_region.end(), payload_text.begin(), payload_text.end());

    MdCtx ctx(EVP_MD_CTX_new());
    size_t len = 0;
    if (!ctx || EVP_DigestSignInit(ctx.get(), nullptr, EVP_sha256(), nullptr, signing_key) != 1 ||
        EVP_DigestSign(ctx.get(), nullptr, &len, signed_region.data(), signed_region.size()) != 1) {
        throw std::runtime_error("signing failed");
    }
    Bytes sig(len);
    if (EVP_DigestSign(ctx.get(), sig.data(), &len, signed_region.data(), signed_region.size()) != 1) {
        throw std::runtime_error("signing failed");
    }
    sig.resize(len);

    const Digest digest = sha256(signed_region);
    const Digest kh = key_hash(signing_key);
    Bytes counter_copy(4, 0);
    put32(counter_copy, 0, counter);     // a copy for fleet tools; NOT covered by the signature
    Bytes records;
    add_tlv(records, kTlvSha256, Bytes(digest.begin(), digest.end()));
    add_tlv(records, kTlvKeyHash, Bytes(kh.begin(), kh.end()));
    add_tlv(records, kTlvSignature, sig);
    add_tlv(records, kTlvInfoCounter, counter_copy);
    Bytes area(4, 0);
    put16(area, 0, kTlvMagic);
    put16(area, 2, uint16_t(4 + records.size()));
    area.insert(area.end(), records.begin(), records.end());

    Bytes image = signed_region;
    image.insert(image.end(), area.begin(), area.end());
    return image;
}

// ---- the device's side ----
struct Parsed {
    Header header;
    Bytes signed_region;
    std::vector<std::pair<uint16_t, Bytes>> tlvs;
    const Bytes* find(uint16_t type) const
    {
        for (const auto& t : tlvs) { if (t.first == type) { return &t.second; } }
        return nullptr;
    }
};

// Every length is checked before it is used: the image comes from outside.
inline std::optional<Parsed> parse(const Bytes& img)
{
    if (img.size() < kHeaderSize || get32(img, 0) != kMagic || get16(img, 4) != kHeaderSize) { return std::nullopt; }
    Parsed p;
    p.header.payload_size = get32(img, 8);
    p.header.major = img[12];
    p.header.minor = img[13];
    p.header.revision = get16(img, 14);
    p.header.security_counter = get32(img, 16);
    const size_t end = size_t(kHeaderSize) + p.header.payload_size;
    if (end + 4 > img.size() || get16(img, end) != kTlvMagic) { return std::nullopt; }
    const size_t total = get16(img, end + 2);
    if (total < 4 || end + total != img.size()) { return std::nullopt; }
    p.signed_region.assign(img.begin(), img.begin() + long(end));
    for (size_t at = end + 4; at < end + total;) {
        if (at + 4 > end + total) { return std::nullopt; }
        const uint16_t type = get16(img, at), len = get16(img, at + 2);
        if (at + 4 + len > end + total) { return std::nullopt; }
        p.tlvs.emplace_back(type, Bytes(img.begin() + long(at + 4), img.begin() + long(at + 4 + len)));
        at += 4 + len;
    }
    return p;
}

struct Verdict {
    bool ok;
    std::string reason;
};

// The checks a boot loader makes before it lets an image run or replace the current one.
inline Verdict validate(const Bytes& img, const std::vector<EVP_PKEY*>& trusted, uint32_t device_counter,
                        bool counter_from_unprotected_tlv = false)
{
    const std::optional<Parsed> p = parse(img);
    if (!p) { return {false, "malformed image"}; }
    const Bytes* digest = p->find(kTlvSha256);
    const Digest actual = sha256(p->signed_region);
    if (digest == nullptr || *digest != Bytes(actual.begin(), actual.end())) {
        return {false, "SHA-256 of header+payload does not match the hash record"};
    }
    const Bytes* kh = p->find(kTlvKeyHash);
    EVP_PKEY* key = nullptr;
    for (EVP_PKEY* k : trusted) {
        const Digest h = key_hash(k);
        if (kh != nullptr && *kh == Bytes(h.begin(), h.end())) { key = k; }
    }
    if (key == nullptr) { return {false, "key hash is not one of the device's trusted keys"}; }
    const Bytes* sig = p->find(kTlvSignature);
    MdCtx ctx(EVP_MD_CTX_new());
    if (sig == nullptr || !ctx || EVP_DigestVerifyInit(ctx.get(), nullptr, EVP_sha256(), nullptr, key) != 1 ||
        EVP_DigestVerify(ctx.get(), sig->data(), sig->size(), p->signed_region.data(), p->signed_region.size()) != 1) {
        return {false, "signature does not verify with the trusted key"};
    }
    uint32_t counter = p->header.security_counter;
    if (counter_from_unprotected_tlv) {           // the forensic lab's bug
        const Bytes* c = p->find(kTlvInfoCounter);
        counter = (c != nullptr && c->size() == 4) ? get32(*c, 0) : 0;
    }
    if (counter < device_counter) {
        return {false, "security counter " + std::to_string(counter) + " < device counter " + std::to_string(device_counter) + " (rollback)"};
    }
    return {true, "valid: version " + std::to_string(p->header.major) + "." + std::to_string(p->header.minor) +
                      ", security counter " + std::to_string(counter)};
}

}  // namespace fw
