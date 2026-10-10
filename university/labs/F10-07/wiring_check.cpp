// F10-07 Listing 3: check a wiring plan against a board's port table before any
// cable is plugged in. The board "U-FC1" and every port, voltage and device below are
// the university's inventions; a real plan uses the real board's documentation.
#include <cstdio>
#include <map>
#include <string>
#include <vector>

enum class Kind { Uart, I2c, Can, Pwm, PowerIn, RcIn };

struct Port
{
    std::string name;
    Kind kind;
    double logicV;   // signal level of the port, volts
    int slots;       // how many devices may share it (I2C and CAN are buses)
};

struct Device
{
    std::string name;
    Kind kind;
    double logicV;
    std::string port; // where the plan connects it
};

const char* kindName(Kind k)
{
    switch (k) {
    case Kind::Uart: return "UART";
    case Kind::I2c: return "I2C";
    case Kind::Can: return "CAN";
    case Kind::Pwm: return "PWM output";
    case Kind::PowerIn: return "power input";
    case Kind::RcIn: return "RC input";
    }
    return "?";
}

int main()
{
    const std::vector<Port> board{{"TELEM1", Kind::Uart, 3.3, 1}, {"TELEM2", Kind::Uart, 3.3, 1},
                                  {"GPS", Kind::Uart, 3.3, 1},    {"I2C", Kind::I2c, 3.3, 4},
                                  {"CAN1", Kind::Can, 0.0, 8},    {"RCIN", Kind::RcIn, 3.3, 1},
                                  {"POWER1", Kind::PowerIn, 0.0, 1}, {"MAIN1", Kind::Pwm, 3.3, 1},
                                  {"MAIN2", Kind::Pwm, 3.3, 1},   {"MAIN3", Kind::Pwm, 3.3, 1},
                                  {"MAIN4", Kind::Pwm, 3.3, 1}};
    const std::vector<Device> plan{{"telemetry radio", Kind::Uart, 3.3, "TELEM1"},
                                   {"GNSS receiver", Kind::Uart, 3.3, "GPS"},
                                   {"external compass", Kind::I2c, 3.3, "I2C"},
                                   {"RC receiver", Kind::RcIn, 5.0, "RCIN"},
                                   {"power module", Kind::PowerIn, 0.0, "POWER1"},
                                   {"ESC 1", Kind::Pwm, 3.3, "MAIN1"},
                                   {"ESC 2", Kind::Pwm, 3.3, "MAIN2"},
                                   {"ESC 3", Kind::Pwm, 3.3, "MAIN4"},
                                   {"ESC 4", Kind::Pwm, 3.3, "MAIN4"},
                                   {"companion computer", Kind::Uart, 3.3, "TELEM3"}};

    std::map<std::string, int> used;
    int problems = 0;
    for (const auto& d : plan) {
        const Port* p = nullptr;
        for (const auto& q : board) {
            if (q.name == d.port) {
                p = &q;
            }
        }
        if (p == nullptr) {
            std::printf("FAIL %-18s -> %-7s no such port on this board\n", d.name.c_str(),
                        d.port.c_str());
            ++problems;
            continue;
        }
        std::string why;
        char buf[96];
        if (p->kind != d.kind) {
            std::snprintf(buf, sizeof buf, " kind: device %s, port %s;", kindName(d.kind),
                          kindName(p->kind));
            why += buf;
        }
        if (p->logicV > 0.0 && d.logicV > p->logicV) {
            std::snprintf(buf, sizeof buf, " signal %.1f V into a %.1f V port;", d.logicV,
                          p->logicV);
            why += buf;
        }
        if (++used[p->name] > p->slots) {
            why += " port already full;";
        }
        std::printf("%s %-18s -> %-7s%s\n", why.empty() ? "ok  " : "FAIL", d.name.c_str(),
                    d.port.c_str(), why.c_str());
        problems += why.empty() ? 0 : 1;
    }
    for (const auto& q : board) {
        if (q.kind == Kind::Pwm && used[q.name] == 0) {
            std::printf("note %-7s is unused: is a motor missing?\n", q.name.c_str());
        }
    }
    std::printf("%d problem(s) found\n", problems);
    return 0;   // a report for the learner; a build-pipeline gate would return 1 on problems
}
