// receiver_fixed.cpp - the receiver of the forensic lab, fixed: it assembles each 16-bit
// field from its two bytes, most significant first, whatever the machine's own order.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

std::vector<std::uint8_t> senderMakes(std::uint16_t seq, const std::string& text)
{
    std::vector<std::uint8_t> p;
    p.push_back(static_cast<std::uint8_t>(seq >> 8));
    p.push_back(static_cast<std::uint8_t>(seq & 0xFF));
    p.push_back(static_cast<std::uint8_t>(text.size() >> 8));
    p.push_back(static_cast<std::uint8_t>(text.size() & 0xFF));
    p.insert(p.end(), text.begin(), text.end());
    return p;
}

struct Header
{
    std::uint16_t seq;
    std::uint16_t length;
};

void receiverHandles(const std::vector<std::uint8_t>& p)
{
    Header h{};
    h.seq = static_cast<std::uint16_t>((p[0] << 8) | p[1]);
    h.length = static_cast<std::uint16_t>((p[2] << 8) | p[3]);
    std::printf("bytes:");
    for (std::uint8_t b : p) {
        std::printf(" %02x", b);
    }
    std::printf("\n  receiver read seq=%u length=%u, packet has %zu payload bytes -> %s\n",
                h.seq, h.length, p.size() - 4,
                h.length == p.size() - 4 ? "accepted" : "REJECTED (length mismatch)");
}

int main()
{
    receiverHandles(senderMakes(1, "temp=21C"));
    receiverHandles(senderMakes(2, "wind=4"));
    receiverHandles(senderMakes(3, "rain=0mm/h"));
    return 0;
}
