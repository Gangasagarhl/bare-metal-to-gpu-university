// channel.cpp - a message cut into packets crosses a pretend best-effort network that
// reorders, duplicates and loses packets; the receiver rebuilds it and asks for gaps.
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

struct Packet
{
    int seq;               // which piece (0, 1, 2, ...)
    int total;             // how many pieces the message has
    std::string payload;   // the piece itself
};

std::vector<Packet> cut(const std::string& message, std::size_t maxPayload)
{
    std::vector<Packet> packets;
    const std::size_t count = (message.size() + maxPayload - 1) / maxPayload;
    for (std::size_t i = 0; i < count; ++i) {
        packets.push_back({static_cast<int>(i), static_cast<int>(count),
                           message.substr(i * maxPayload, maxPayload)});
    }
    return packets;
}

// Best effort: each packet may be lost (1 in 8) or duplicated (1 in 8); neighbours may swap.
std::vector<Packet> network(const std::vector<Packet>& sent, std::mt19937& rng)
{
    std::vector<Packet> arrived;
    for (const Packet& p : sent) {
        const std::uint32_t r = rng() % 8;
        if (r == 0) {
            continue;                          // lost
        }
        arrived.push_back(p);
        if (r == 1) {
            arrived.push_back(p);              // delivered twice
        }
    }
    for (std::size_t i = 0; i + 1 < arrived.size(); ++i) {
        if (rng() % 3 == 0) {
            std::swap(arrived[i], arrived[i + 1]);   // overtaken on the way
        }
    }
    return arrived;
}

void show(const char* label, const std::vector<Packet>& packets)
{
    std::cout << label;
    for (const Packet& p : packets) {
        std::cout << ' ' << p.seq;
    }
    std::cout << '\n';
}

int main()
{
    const std::string message = "Meet at the library at four. Bring the blue notebook.";
    const std::vector<Packet> sent = cut(message, 8);
    std::mt19937 rng(2026);                    // fixed seed: the same run every time

    std::map<int, std::string> received;       // seq -> payload; sorted; duplicates ignored
    std::vector<Packet> toSend = sent;
    for (int round = 1; !toSend.empty() && round <= 5; ++round) {
        std::cout << "round " << round << '\n';
        show("  sent seq:   ", toSend);
        const std::vector<Packet> arrived = network(toSend, rng);
        show("  arrived seq:", arrived);
        for (const Packet& p : arrived) {
            if (!received.emplace(p.seq, p.payload).second) {
                std::cout << "  duplicate of " << p.seq << " ignored\n";
            }
        }
        toSend.clear();
        for (const Packet& p : sent) {
            if (received.count(p.seq) == 0) {
                toSend.push_back(p);           // receiver asks again for every gap
            }
        }
        show("  missing:    ", toSend);
    }

    std::string rebuilt;
    for (const auto& [seq, payload] : received) {
        rebuilt += payload;
    }
    std::cout << "packets: " << sent.size() << ", rebuilt: \"" << rebuilt << "\"\n";
    std::cout << (rebuilt == message ? "identical to the original\n" : "NOT identical\n");
    return 0;
}
