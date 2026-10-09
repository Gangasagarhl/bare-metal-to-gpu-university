// measure.cc - the arithmetic of measured boot, without a TPM: hash each boot component with
// SHA-256 and "extend" one register with each digest: PCR_new = SHA-256(PCR_old || digest).
//   measure <file> ...   prints each file's digest, then the register after each extend, then the
//                        register for the same files in reverse order.
// SHA-256 is written out here (FIPS 180-4 algorithm, from memory); run.sh compares every
// digest with sha256sum and every extend with Python's hashlib, so the code is checked by runs.
#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

using Digest = std::array<std::uint8_t, 32>;

std::uint32_t rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

Digest sha256(const std::vector<std::uint8_t>& msg)
{
    static const std::uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    std::uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                          0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::vector<std::uint8_t> m = msg;               // padding: 0x80, zeros, 64-bit bit length
    const std::uint64_t bits = static_cast<std::uint64_t>(msg.size()) * 8;
    m.push_back(0x80);
    while (m.size() % 64 != 56) {
        m.push_back(0);
    }
    for (int i = 7; i >= 0; --i) {
        m.push_back(static_cast<std::uint8_t>(bits >> (8 * i)));
    }
    for (std::size_t block = 0; block < m.size(); block += 64) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            const std::size_t p = block + static_cast<std::size_t>(i) * 4;
            w[i] = static_cast<std::uint32_t>(m[p]) << 24 | m[p + 1] << 16 | m[p + 2] << 8 | m[p + 3];
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t t1 = hh + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i];
            const std::uint32_t t2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            hh = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    Digest out{};
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 4; ++j) {
            out[static_cast<std::size_t>(i * 4 + j)] = static_cast<std::uint8_t>(h[i] >> (24 - 8 * j));
        }
    }
    return out;
}

std::string text(const Digest& d)
{
    std::string s;
    char buf[3];
    for (const std::uint8_t byte : d) {
        std::snprintf(buf, sizeof buf, "%02x", byte);
        s += buf;
    }
    return s;
}

Digest extend(const Digest& pcr, const Digest& measurement)
{
    std::vector<std::uint8_t> joined(pcr.begin(), pcr.end());
    joined.insert(joined.end(), measurement.begin(), measurement.end());
    return sha256(joined);
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::printf("usage: measure <file> ...\n");
        return 2;
    }
    std::vector<Digest> digests;
    for (int i = 1; i < argc; ++i) {
        std::ifstream in(argv[i], std::ios::binary);
        const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (!in.good() && !in.eof()) {
            std::printf("cannot read %s\n", argv[i]);
            return 1;
        }
        digests.push_back(sha256(data));
        std::printf("%s  %s\n", text(digests.back()).c_str(), argv[i]);
    }
    Digest pcr{};  // a PCR starts as 32 zero bytes in this model
    std::printf("PCR start            %s\n", text(pcr).c_str());
    for (std::size_t i = 0; i < digests.size(); ++i) {
        pcr = extend(pcr, digests[i]);
        std::printf("after extend %-7zu %s\n", i + 1, text(pcr).c_str());
    }
    Digest reversed{};
    for (std::size_t i = digests.size(); i-- > 0;) {
        reversed = extend(reversed, digests[i]);
    }
    std::printf("same files, reverse order: %s\n", text(reversed).c_str());
    return 0;
}
