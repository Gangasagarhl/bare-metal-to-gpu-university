// F11-15 forensic evidence generator: "The robot that obeyed an old command".
// Policy MacFresh is deployed. During a demo the motor controller restarts after a
// battery-voltage dip. Minutes later the robot drives off on its own. This program
// prints the controller's log. SYNTHETIC times and frames; the verdicts are computed
// by robotlink.h exactly as in Listing 6.
#include <cstdio>
#include <string>
#include <vector>

#include "robotlink.h"

int main()
{
    const std::string key = "robot-key-for-the-lab";
    robot::Controller c;
    c.policy = robot::Policy::MacFresh;
    c.key = key;
    struct Line
    {
        const char* time;
        const char* src;
        std::string frame;   // empty: controller restart
    };
    std::vector<Line> lines;
    const char* bodies[] = {
        "seq=101;cmd=drive;v=0.20;w=0.00", "seq=102;cmd=drive;v=0.20;w=0.30",
        "seq=103;cmd=stop;v=0.00;w=0.00",  "seq=104;cmd=drive;v=0.40;w=0.00",
        "seq=105;cmd=stop;v=0.00;w=0.00"};
    const char* times[] = {"14:00:05", "14:00:09", "14:00:15", "14:01:30", "14:01:41"};
    for (int i = 0; i < 5; ++i) {
        lines.push_back({times[i], "10.0.0.20", robot::frame(key, bodies[i])});
    }
    lines.push_back({"14:02:10", "-", ""});
    lines.push_back({"14:02:31", "10.0.0.20", robot::frame(key, "seq=1;cmd=stop;v=0.00;w=0.00")});
    lines.push_back({"14:02:58", "10.0.0.20", robot::frame(key, "seq=2;cmd=drive;v=0.20;w=0.00")});
    lines.push_back({"14:03:02", "10.0.0.20", robot::frame(key, "seq=3;cmd=stop;v=0.00;w=0.00")});
    lines.push_back({"14:06:44", "10.0.0.77", robot::frame(key, bodies[3])});
    lines.push_back({"14:06:51", "10.0.0.20", robot::frame(key, "seq=4;cmd=stop;v=0.00;w=0.00")});
    lines.push_back({"14:06:52", "10.0.0.20", robot::frame(key, "seq=5;cmd=stop;v=0.00;w=0.00")});
    std::printf("motor-controller log (policy MacFresh, 8-byte tags)\n");
    std::printf("time      source      frame                                  verdict\n");
    for (const Line& l : lines) {
        if (l.frame.empty()) {
            c.restart();
            std::printf("%s  -           controller restart (supply dip), state reset\n", l.time);
            continue;
        }
        const std::string body = l.frame.substr(0, l.frame.find("|tag="));
        const std::string verdict = c.receive(l.frame);
        std::printf("%s  %-10s  %-37s  %s\n", l.time, l.src, body.c_str(), verdict.c_str());
    }
    return 0;
}
