// F11-15 Listing 6: the same eight frames reach the motor controller under three policies.
// The attacker is on the robot's network: it can read every frame and send its own, but
// it does not know the key. SYNTHETIC scenario; the verdicts are computed by the code.
#include <cstdio>
#include <string>
#include <vector>

#include "robotlink.h"

struct Event
{
    const char* who;
    std::string frame;
};

int main()
{
    const std::string key = "robot-key-for-the-lab";
    const std::string f1 = robot::frame(key, "seq=1;cmd=drive;v=0.30;w=0.00");
    const std::string f2 = robot::frame(key, "seq=2;cmd=drive;v=0.00;w=0.50");
    const std::string f3 = robot::frame(key, "seq=3;cmd=stop;v=0.00;w=0.00");
    const std::vector<Event> events = {
        {"operator", f1},
        {"operator", f2},
        {"attacker", "seq=4;cmd=drive;v=1.50;w=0.00"},                         // forged, no tag
        {"attacker", "seq=4;cmd=drive;v=1.50;w=0.00|tag=0000000000000000"},    // guessed tag
        {"attacker", "seq=4;cmd=drive;v=1.50;w=0.00" + f1.substr(f1.find("|tag="))}, // tag moved
        {"operator", f3},
        {"attacker", f1},                                                       // replay of frame 1
        {"attacker", f2},                                                       // replay of frame 2
    };
    const char* names[] = {"Plain", "Mac", "MacFresh"};
    const robot::Policy pols[] = {robot::Policy::Plain, robot::Policy::Mac,
                                  robot::Policy::MacFresh};
    for (int p = 0; p < 3; ++p) {
        robot::Controller c;
        c.policy = pols[p];
        c.key = key;
        std::printf("policy %s\n", names[p]);
        int i = 0;
        int attackerObeyed = 0;
        for (const Event& e : events) {
            const std::string v = c.receive(e.frame);
            if (v == "OBEYED" && std::string(e.who) == "attacker") {
                ++attackerObeyed;
            }
            const std::size_t bar = e.frame.find("|tag=");
            const std::string body = e.frame.substr(0, bar);
            const std::string tag = bar == std::string::npos ? "(none)" : e.frame.substr(bar + 5);
            std::printf("  %d %-8s %-30s %-16s %s\n", ++i, e.who, body.c_str(), tag.c_str(),
                        v.c_str());
        }
        std::printf("  obeyed %d frames, %d of them from the attacker\n", c.obeyed, attackerObeyed);
    }
    std::printf("frame 1 on the wire: %s\n", f1.c_str());
    return 0;
}
