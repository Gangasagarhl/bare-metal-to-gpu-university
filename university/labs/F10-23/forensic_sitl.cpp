// forensic_sitl.cpp - evidence for the F10-23 forensic lab: the same autopilot,
// connected to a new version of the simulator bridge.
#include "sitl_core.h"

int main()
{
    return sitl::runSitl(true);
}
