// phantom_fix.cpp - F9-52 forensic answer key: the same mapping run as map_house.cpp,
// after the LiDAR range setting was lowered to 2.0 m. Prints the first scan's raw
// ranges in one direction, then the map and its score.
#include "occgrid.hpp"
#include <cstdio>

int main()
{
    const rb::Grid truth = rb::makeHouse();
    rb::LidarSpec lidar;
    lidar.maxRange = 2.0;               // the new LiDAR setting
    rb::Rng rng(52);
    const std::vector<rb::Pose> path = rb::densify(rb::routeWaypoints(), 0.05);

    rb::GridParams gp;
    gp.maxRangeIsHit = false;           // the fix: a beam with no return marks no obstacle
    rb::OccGrid map(gp);
    int scans = 0;
    double sinceScan = 1e9;
    for (std::size_t k = 0; k < path.size(); ++k) {
        if (k > 0) { sinceScan += std::hypot(path[k].x - path[k - 1].x, path[k].y - path[k - 1].y); }
        if (sinceScan < 0.25) { continue; }
        const std::vector<double> z = rb::scan(truth, path[k], lidar, rng);
        if (scans == 0) {
            std::printf("scan 0 at x=%.2f y=%.2f th=%.2f; beams 20..40 (degrees from heading):\n",
                        path[k].x, path[k].y, path[k].th);
            for (int b = 20; b <= 40; b += 4) { std::printf("  beam %3d: %.3f m\n", b, z[b]); }
        }
        map.integrate(path[k], z, lidar.maxRange);
        ++scans;
        sinceScan = 0.0;
    }
    const rb::Score s = rb::compare(map, truth);
    std::printf("%d scans; known %d cells, wrong 'O' %d (far from walls %d), wrong '.' %d\n",
                scans, s.known, s.falseOcc, s.falseOccFar, s.falseFree);
    rb::printMap(map, true);
    return 0;
}
