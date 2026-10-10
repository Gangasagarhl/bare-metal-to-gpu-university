// dev.cpp - SE301 F12-05 forensic: status polling of a simulated storage device.
// A removed device reads as all ones (the convention of curriculum 5.4, "surprise
// removal"), so its BUSY bit looks set forever. Each stdin line is one scenario:
//   normal | unplug-before-reset | unplug-before-wait
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

constexpr std::uint32_t kBusy = 0x1;
constexpr std::uint32_t kReady = 0x2;

struct Device
{
    bool present = true;
    int busyPolls = 3;                                  // busy for 3 reads after a command
    std::uint32_t readStatus()
    {
        if (!present) return 0xFFFFFFFF;
        if (busyPolls > 0) {
            --busyPolls;
            return kBusy;
        }
        return kReady;
    }
    void command() { busyPolls = 3; }
};

static bool resetDevice(Device& d)
{
    d.command();
    while (d.readStatus() & kBusy) {
    }
    return true;
}

static bool waitIdle(Device& d)
{
    while (d.readStatus() & kBusy) {
    }
    return true;
}

static void say(const char* s)
{
    std::printf("%s\n", s);
    std::fflush(stdout);                                // keep output if we are killed
}

int main()
{
    std::string scenario;
    while (std::cin >> scenario) {
        Device d;
        std::printf("scenario %s\n", scenario.c_str());
        if (scenario == "unplug-before-reset") d.present = false;
        if (!resetDevice(d)) {
            say("  reset: FAILED (device gone?)");
            continue;
        }
        say("  reset: ok");
        d.command();
        if (scenario == "unplug-before-wait") d.present = false;
        say(waitIdle(d) ? "  wait:  ok" : "  wait:  FAILED (device gone?)");
    }
    return 0;
}
