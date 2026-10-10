// chain.cpp - F11-01 Listing 2: seven scenarios on the hash-locked chain of chain_model.h:
// what the chain stops, what it cannot stop, and why real chains use signatures instead of
// fixed digests.
#include "chain_model.h"

int main()
{
    const std::vector<Stage> factory = build_release("app v1: drive the robot");
    const Digest rom = digest_of(factory[0]);    // burned into the ROM at manufacture
    std::cout << "ROM holds the digest of bl1: " << short_hex(rom) << " (cannot be changed)\n\n";

    std::cout << "1. Factory image\n";
    boot(rom, factory);

    std::cout << "2. Attacker changes the app\n";
    std::vector<Stage> c2 = factory;
    c2[3].code = "app v1: drive the robot AND send camera images away";
    boot(rom, c2);

    std::cout << "3. Attacker changes the os and patches bl2's expected digest to match\n";
    std::vector<Stage> c3 = factory;
    c3[2].code = "os v1 with a hidden remote shell";
    c3[1].expected_next = digest_of(c3[2]);
    boot(rom, c3);

    std::cout << "4. Attacker rebuilds bl1, bl2, os and app consistently\n";
    std::vector<Stage> c4 = build_release("app v1: drive the robot AND send camera images away");
    c4[0].code = "bl1 v1 (attacker's copy): set up memory, load bl2";
    boot(rom, c4);

    std::cout << "5. Devices made with a bl2 whose check was left out (a debug build shipped by mistake)\n";
    std::vector<Stage> c5 = build_release("app v1: drive the robot", false);
    const Digest rom5 = digest_of(c5[0]);        // these devices' ROM holds this release's bl1
    boot(rom5, c5);
    std::cout << "   ... and now the os is replaced on such a device\n";
    c5[2].code = "os v1 with a hidden remote shell";
    boot(rom5, c5);

    std::cout << "6. The vendor ships a legitimate app v2\n";
    const std::vector<Stage> v2 = build_release("app v2: drive the robot, faster");
    for (int i = 0; i < 4; ++i) {
        std::cout << "    " << v2[i].name << " digest v1 " << short_hex(digest_of(factory[i])) << "  v2 "
                  << short_hex(digest_of(v2[i])) << (digest_of(factory[i]) == digest_of(v2[i]) ? "  same\n" : "  CHANGED\n");
    }
    boot(rom, v2);

    std::cout << "7. Self-test: SHA-256(\"abc\") = ";
    Sha256 h;
    const std::string abc = "abc";
    h.update(reinterpret_cast<const uint8_t*>(abc.data()), abc.size());
    Digest d{};
    h.finish(d.data());
    for (uint8_t b : d) {
        std::cout << "0123456789abcdef"[b >> 4] << "0123456789abcdef"[b & 0xf];
    }
    std::cout << "\n";
    return 0;
}
