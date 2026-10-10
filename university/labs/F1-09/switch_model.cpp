// Ideal-switch model of transistors: which switches conduct, and what the output becomes.
#include <iostream>
#include <string>

// Our model's rules: nMOS conducts when its gate is 1; pMOS conducts when its gate is 0.
bool nmosOn(int gate)
{
    return gate == 1;
}

bool pmosOn(int gate)
{
    return gate == 0;
}

// The output wire is joined to the supply by the pull-up switch
// and to ground by the pull-down switch.
std::string outputOf(bool pullUpOn, bool pullDownOn)
{
    if (pullUpOn && pullDownOn) {
        return "X  (both conduct: supply shorted to ground)";
    }
    if (pullUpOn) {
        return "1  (joined to the supply)";
    }
    if (pullDownOn) {
        return "0  (joined to ground)";
    }
    return "Z  (floating: joined to nothing)";
}

std::string onOff(bool on)
{
    return on ? "ON " : "off";
}

int main()
{
    std::cout << "CMOS inverter: pMOS pull-up, nMOS pull-down\n";
    std::cout << "in  pMOS  nMOS  out\n";
    for (int in = 0; in <= 1; ++in) {
        const bool up = pmosOn(in);
        const bool down = nmosOn(in);
        std::cout << in << "   " << onOff(up) << "   " << onOff(down) << "   "
                  << outputOf(up, down) << '\n';
    }

    std::cout << "\nWrong build: nMOS used as the pull-up too\n";
    std::cout << "in  up    nMOS  out\n";
    for (int in = 0; in <= 1; ++in) {
        const bool up = nmosOn(in);
        const bool down = nmosOn(in);
        std::cout << in << "   " << onOff(up) << "   " << onOff(down) << "   "
                  << outputOf(up, down) << '\n';
    }
    return 0;
}
