// bt_bug.cpp - F9-58 forensic evidence: the same mission, built with the GoTo action from
// a colleague's branch (see the answer key). Battery 60%, kitchen door closed.
#include "mission.hpp"

int main()
{
    World w;
    return runMission(w, true) == bt::Status::Success ? 0 : 3;
}
