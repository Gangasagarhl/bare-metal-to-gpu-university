// F11-13 forensic evidence generator: "The door nobody listed".
// A home robot keeps a motor log of every command it obeyed, with the entry point the
// command came in through. A monitor was built from the household's threat model: it
// watches only the entry points the model lists. SYNTHETIC: the household, the times and
// the entry-point names are exercise values; what is real is that this program ran and
// printed both logs below.
#include <cstdio>
#include <string>
#include <vector>

struct Cmd
{
    const char* time;
    const char* entry;
    const char* what;
};

int main()
{
    const std::vector<std::string> modelEntries = {"wifi-command-port", "buttons", "service-port"};
    const std::vector<Cmd> cmds = {
        {"18:02:11", "buttons", "drive forward 40 cm"},
        {"18:05:37", "wifi-command-port", "turn left 90 deg"},
        {"18:05:52", "wifi-command-port", "drive forward 120 cm"},
        {"21:30:04", "wifi-command-port", "dock"},
        {"02:14:09", "bluetooth-setup", "undock"},
        {"02:14:21", "bluetooth-setup", "drive forward 300 cm"},
        {"02:14:58", "bluetooth-setup", "open camera stream"},
        {"07:45:10", "buttons", "dock"},
    };
    std::printf("== threat model (household, written from the robot's manual) ==\n");
    for (const std::string& e : modelEntries) {
        std::printf("entry point: %s\n", e.c_str());
    }
    std::printf("\n== monitor log (watches the entry points of the threat model) ==\n");
    int seen = 0;
    for (const Cmd& c : cmds) {
        for (const std::string& e : modelEntries) {
            if (e == c.entry) {
                std::printf("%s  %-18s %s\n", c.time, c.entry, c.what);
                ++seen;
            }
        }
    }
    std::printf("monitor summary: %d commands seen, 0 alerts\n", seen);
    std::printf("\n== robot motor log (every command the robot obeyed) ==\n");
    for (const Cmd& c : cmds) {
        std::printf("%s  %-18s %s\n", c.time, c.entry, c.what);
    }
    std::printf("motor log summary: %zu commands obeyed\n", cmds.size());
    return 0;
}
