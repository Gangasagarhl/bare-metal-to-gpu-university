// map_house.cpp - F9-52 lab: map the simulated house with known poses.
// The robot drives the route of house.hpp; every 0.25 m it takes a LiDAR scan,
// and the occupancy grid adds the scan at the TRUE pose (mapping with known poses).
#include "occgrid.hpp"
#include <cstdio>

int main()
{
    const rb::Grid truth = rb::makeHouse();
    const rb::LidarSpec lidar;                      // 360 beams, 4.0 m, sigma 0.02 m
    rb::Rng rng(52);
    const std::vector<rb::Pose> path = rb::densify(rb::routeWaypoints(), 0.05);

    rb::OccGrid map;
    int scans = 0;
    double travelled = 0.0;
    double sinceScan = 1e9;                         // take a scan at the start
    for (std::size_t k = 0; k < path.size(); ++k) {
        if (k > 0) {
            const double d = std::hypot(path[k].x - path[k - 1].x, path[k].y - path[k - 1].y);
            travelled += d;
            sinceScan += d;
        }
        if (sinceScan >= 0.25) {
            map.integrate(path[k], rb::scan(truth, path[k], lidar, rng), lidar.maxRange);
            ++scans;
            sinceScan = 0.0;
            if (scans == 1 || scans == 20 || scans == 60) {
                const rb::Score s = rb::compare(map, truth);
                std::printf("after scan %3d: known %4d, wrong 'O' %3d (far from walls %2d), wrong '.' %d\n",
                            scans, s.known, s.falseOcc, s.falseOccFar, s.falseFree);
            }
        }
    }
    const rb::Score s = rb::compare(map, truth);
    std::printf("route %.2f m, %d scans of %d beams\n", travelled, scans, lidar.beams);
    std::printf("final: known %d of %d cells, wrong 'O' %d (far from walls %d), wrong '.' %d,"
                " correct 'O' %d\n", s.known, rb::kW * rb::kH, s.falseOcc, s.falseOccFar, s.falseFree,
                s.occCorrect);
    std::printf("one wall cell (i=40,j=5):  log-odds %+.2f  p=%.3f\n", map.logOdds(40, 5), map.prob(40, 5));
    std::printf("one floor cell (i=20,j=20): log-odds %+.2f  p=%.3f\n", map.logOdds(20, 20), map.prob(20, 20));
    std::printf("map (each character = 2 x 2 cells = 0.2 m x 0.2 m; O occupied, . free, blank unknown):\n");
    rb::printMap(map, true);
    return 0;
}
