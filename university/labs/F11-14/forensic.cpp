// F11-14 forensic evidence generator: "Zero threats at the boundary".
// Two versions of the same team's diagram are run through the engine of Listing 1.
// Version A was reviewed and signed off. Version B is the earlier draft the reviewer
// compared it with. The robot later obeyed schedules sent by a stranger.
// SYNTHETIC diagrams written for the exercise; the outputs are the engine's real output.
#include <cstdio>
#include <sstream>

#include "tm.h"

int main()
{
    const char* a =
        "title Version A (signed off)\n"
        "external home Gardener\n"
        "process  home Phone app\n"
        "process  home Relay service\n"
        "process  home Robot controller\n"
        "flow Gardener -> Phone app : taps\n"
        "flow Phone app -> Relay service : schedule\n"
        "flow Relay service -> Robot controller : schedule\n";
    const char* b =
        "title Version B (earlier draft)\n"
        "external phone Gardener\n"
        "process  phone Phone app\n"
        "process  cloud Relay service\n"
        "process  home  Robot controller\n"
        "flow Gardener -> Phone app : taps\n"
        "flow Phone app -> Relay service : schedule\n"
        "flow Relay service -> Robot controller : schedule\n";
    for (const char* text : {a, b}) {
        std::istringstream in(text);
        const tmod::Model m = tmod::parse(in);
        const int rc = tmod::report(m);
        std::printf("engine exit code: %d\n\n", rc);
    }
    return 0;
}
