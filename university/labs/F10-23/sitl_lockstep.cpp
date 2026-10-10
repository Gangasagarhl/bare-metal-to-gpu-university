// sitl_lockstep.cpp - run the two-process SITL of sitl_core.h: take off to 2 m and hold.
#include "sitl_core.h"

int main()
{
    return sitl::runSitl(false);
}
