// F1-47 Listing 4: CAN arbitration. Three nodes start a frame at the same time
// and send their 11-bit identifiers MSB first. The bus is a wired-AND: a 0
// (dominant) from any node wins over a 1 (recessive). A node that sends 1 but
// reads 0 back has lost and stops sending; the lowest identifier wins without
// any frame being destroyed. 11 bits = CAN_SFF_ID_BITS from linux/can.h.
#include <linux/can.h>

#include <cstdint>
#include <cstdio>
#include <vector>

int main()
{
    struct Node
    {
        const char* name;
        std::uint32_t id;
        bool sending = true;
    };
    std::vector<Node> nodes{{"engine", 0x65A}, {"brakes", 0x123}, {"wipers", 0x124}};
    for (Node& n : nodes) n.id &= CAN_SFF_MASK;
    std::printf("bit  engine brakes wipers  bus\n");
    for (int bit = CAN_SFF_ID_BITS - 1; bit >= 0; --bit) {
        int bus = 1;
        for (const Node& n : nodes) {
            if (n.sending) bus &= static_cast<int>((n.id >> bit) & 1u);   // wired-AND
        }
        std::printf("%2d  ", bit);
        for (Node& n : nodes) {
            const int mine = static_cast<int>((n.id >> bit) & 1u);
            if (!n.sending) {
                std::printf("   .   ");
                continue;
            }
            std::printf("   %d   ", mine);
            if (mine == 1 && bus == 0) n.sending = false;   // lost arbitration
        }
        std::printf("  %d\n", bus);
    }
    for (const Node& n : nodes) {
        std::printf("%-7s id 0x%03X  %s\n", n.name, static_cast<unsigned>(n.id),
                    n.sending ? "WON: its frame continues" : "lost: retries after this frame");
    }
    return 0;
}
