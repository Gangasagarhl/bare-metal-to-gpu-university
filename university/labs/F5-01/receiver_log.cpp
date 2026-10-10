// receiver_log.cpp - evidence for the forensic lab "Every packet rejected".
// The sender writes its header in network byte order (most significant byte first).
// The receiver below copies the header bytes straight into its own integers.
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
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
    std::memcpy(&h.seq, p.data(), 2);
    std::memcpy(&h.length, p.data() + 2, 2);
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
