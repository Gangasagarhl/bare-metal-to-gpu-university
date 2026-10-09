// F0-79 forensic evidence: a joint position loop after the firmware's loop period changed from 1 ms
// to 10 ms. The model and controller are in buzz.hpp; the deliberate mistake is described in the
// answer key.
#include "buzz.hpp"

int main()
{
    run("last week's build", 0.001, kDtInCode);
    run("this week's build", 0.010, kDtInCode);
    return 0;
}
