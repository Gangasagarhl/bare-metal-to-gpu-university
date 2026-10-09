// sha256.h - DR302 F4-13: SHA-256 (FIPS 180-4), header-only and freestanding, so the same
// code runs in the lab kernel and in the host tools. Checked in this build against the
// FIPS example digests and against Python's hashlib (sha256check, run.sh).
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace sha {
struct Sha256 {
    uint32_t h[8];
    uint8_t block[64];
    uint64_t total;          // bytes hashed so far
    uint32_t used;           // bytes waiting in 'block'
};

namespace detail {
constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
inline uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

inline void compress(uint32_t h[8], const uint8_t b[64])
{
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)                 // the message block is big-endian
        w[i] = (uint32_t{b[4 * i]} << 24) | (uint32_t{b[4 * i + 1]} << 16) | (uint32_t{b[4 * i + 2]} << 8) | b[4 * i + 3];
    for (int i = 16; i < 64; ++i) {
        const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = h[0], bb = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 64; ++i) {
        const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t t1 = hh + S1 + ch + K[i] + w[i];
        const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const uint32_t maj = (a & bb) ^ (a & c) ^ (bb & c);
        const uint32_t t2 = S0 + maj;
        hh = g; g = f; f = e; e = d + t1; d = c; c = bb; bb = a; a = t1 + t2;
    }
    h[0] += a; h[1] += bb; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
}
}  // namespace detail

inline void init(Sha256& s)
{
    static constexpr uint32_t IV[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                       0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    for (int i = 0; i < 8; ++i) s.h[i] = IV[i];
    s.total = 0;
    s.used = 0;
}

inline void update(Sha256& s, const void* data, size_t n)
{
    const auto* p = static_cast<const uint8_t*>(data);
    s.total += n;
    while (n) {
        const uint32_t take = (64 - s.used) < n ? (64 - s.used) : static_cast<uint32_t>(n);
        for (uint32_t i = 0; i < take; ++i) s.block[s.used + i] = p[i];
        s.used += take;
        p += take;
        n -= take;
        if (s.used == 64) { detail::compress(s.h, s.block); s.used = 0; }
    }
}

// Padding: a 1 bit, zeros, then the message length in bits as a 64-bit big-endian number,
// so that the total is a multiple of 64 bytes.
inline void final(Sha256& s, uint8_t out[32])
{
    const uint64_t bits = s.total * 8;
    const uint8_t one = 0x80, zero = 0;
    update(s, &one, 1);
    while (s.used != 56) update(s, &zero, 1);
    uint8_t len[8];
    for (int i = 0; i < 8; ++i) len[i] = static_cast<uint8_t>(bits >> (56 - 8 * i));
    update(s, len, 8);
    for (int i = 0; i < 8; ++i)
        for (int k = 0; k < 4; ++k) out[4 * i + k] = static_cast<uint8_t>(s.h[i] >> (24 - 8 * k));
}

inline void hash(const void* data, size_t n, uint8_t out[32])
{
    Sha256 s;
    init(s);
    update(s, data, n);
    final(s, out);
}

// PCR extend: new = SHA-256(old || digest). A PCR can only be extended, never written.
inline void extend(uint8_t pcr[32], const uint8_t digest[32])
{
    Sha256 s;
    init(s);
    update(s, pcr, 32);
    update(s, digest, 32);
    final(s, pcr);
}
}  // namespace sha
