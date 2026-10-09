// header_bytes.cpp - put a small packet header into bytes in network byte order,
// add an Internet-style checksum, and show that a damaged byte is detected.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// Append a 16-bit number most significant byte first ("network byte order").
void put16(std::vector<std::uint8_t>& out, std::uint16_t value)
{
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value & 0xFF));
}

// Read a 16-bit number stored most significant byte first.
std::uint16_t get16(const std::vector<std::uint8_t>& in, std::size_t at)
{
    return static_cast<std::uint16_t>((in[at] << 8) | in[at + 1]);
}

// Ones' complement sum of 16-bit words, then complemented (the Internet checksum idea).
std::uint16_t checksum(const std::vector<std::uint8_t>& bytes)
{
    std::uint32_t sum = 0;
    for (std::size_t i = 0; i < bytes.size(); i += 2) {
        std::uint32_t word = static_cast<std::uint32_t>(bytes[i]) << 8;
        if (i + 1 < bytes.size()) {
            word |= bytes[i + 1];
        }
        sum += word;
        sum = (sum & 0xFFFF) + (sum >> 16);   // fold the carry back in
    }
    return static_cast<std::uint16_t>(~sum & 0xFFFF);
}

// Header: seq (2 bytes), total (2), payload length (2), checksum (2), then the payload.
std::vector<std::uint8_t> makePacket(std::uint16_t seq, std::uint16_t total,
                                     const std::string& payload)
{
    std::vector<std::uint8_t> p;
    put16(p, seq);
    put16(p, total);
    put16(p, static_cast<std::uint16_t>(payload.size()));
    put16(p, 0);                               // checksum field is zero while computing
    p.insert(p.end(), payload.begin(), payload.end());
    const std::uint16_t c = checksum(p);
    p[6] = static_cast<std::uint8_t>(c >> 8);
    p[7] = static_cast<std::uint8_t>(c & 0xFF);
    return p;
}

void dump(const char* label, const std::vector<std::uint8_t>& p)
{
    std::printf("%-9s", label);
    for (std::uint8_t b : p) {
        std::printf(" %02x", b);
    }
    std::printf("\n");
}

// The receiver's test: the checksum over the whole packet, field included, must be 0.
bool looksIntact(const std::vector<std::uint8_t>& p)
{
    return checksum(p) == 0;
}

int main()
{
    std::vector<std::uint8_t> p = makePacket(2, 5, "Hi Amara");
    dump("packet:", p);
    std::printf("seq=%u total=%u length=%u checksum=0x%04x\n",
                get16(p, 0), get16(p, 2), get16(p, 4), get16(p, 6));
    std::printf("receiver check on the intact packet: %s\n", looksIntact(p) ? "ok" : "DAMAGED");

    p[9] ^= 0x04;                              // one bit flips on the way ('i' becomes 'm')
    dump("damaged:", p);
    std::printf("receiver check on the damaged packet: %s\n", looksIntact(p) ? "ok" : "DAMAGED");
    return 0;
}
