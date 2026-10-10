// controller_main.cc - the controller as its own program, as it would run on the robot's
// computer: it reads range packets "R <mm>" on standard input and answers each with a speed
// command "C <m/s>" on standard output. Built twice: for the build machine (x86-64) and for
// an AArch64 target, which runs under QEMU user-mode emulation (processor-in-the-loop).
#include "../F12-06/speedctl.hpp"

#include <cstdio>
#include <cstring>
#include <string_view>

int main()
{
    char line[64];
    while (std::fgets(line, sizeof line, stdin) != nullptr) {
        std::string_view packet(line, std::strcspn(line, "\n"));
        if (packet == "Q") {
            break;
        }
        const auto d = speedctl::parse_range_m(packet);
        const double cmd = d ? speedctl::governor(*d) : 0.0;  // unparsable packet: stop
        std::printf("C %.6f\n", cmd);
        std::fflush(stdout);  // the plant waits for this line: never leave it in a buffer
    }
    return 0;
}
