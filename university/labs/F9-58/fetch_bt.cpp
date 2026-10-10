// fetch_bt.cpp - F9-58 lab: the fetch-a-cup mission as a behaviour tree, twice:
// run 1 with a full battery; run 2 with the battery nearly at its limit.
#include "mission.hpp"

int main()
{
    std::printf("run 1: battery 60%%, kitchen door closed\n");
    World w1;
    const bt::Status s1 = runMission(w1, false);
    std::printf("\nrun 2: battery 26%%, kitchen door closed\n");
    World w2;
    w2.battery = 26.0;
    const bt::Status s2 = runMission(w2, false);
    return (s1 == bt::Status::Success && s2 == bt::Status::Success) ? 0 : 1;
}
