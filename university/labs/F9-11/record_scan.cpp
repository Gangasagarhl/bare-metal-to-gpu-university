// F9-11 Listing 1: "record" one scan from the simulated LiDAR into the university's text
// scan log (a stand-in for a recorded bag; it is NOT the ROS 2 bag format).
// The robot stands at the world origin facing +x; the LiDAR is mounted 0.10 m ahead
// of the robot's centre (pretend values).
#include <cstdio>

#include "scan_sim.h"

int main()
{
    const ScanSpec spec;
    const double mountX = 0.10;
    const auto r = scan(mountX, 0.0, 0.0, spec, 12345u);
    std::printf("# university scan log v1 (text; not a ROS bag). 0 = no return.\n");
    std::printf("mount x %.3f y %.3f yaw_deg %.1f\n", mountX, 0.0, 0.0);
    std::printf("scan t %.3f angle_min_deg %.1f angle_inc_deg %.1f range_min %.3f range_max %.3f "
                "count %d\n",
                12.400, spec.angleMinDeg, spec.angleIncDeg, spec.rangeMin, spec.rangeMax,
                spec.count);
    for (std::size_t i = 0; i < r.size(); ++i) {
        std::printf("%.3f%s", r[i], (i % 10 == 9) ? "\n" : " ");
    }
    std::printf("end\n");
    return 0;
}
