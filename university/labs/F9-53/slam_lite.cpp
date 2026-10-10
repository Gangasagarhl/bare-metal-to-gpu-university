// slam_lite.cpp - F9-53 lab: map the house WITHOUT knowing the true pose.
// Run A: put each scan at the pose that wheel odometry says (dead reckoning).
// Run B: predict with odometry, then correct by matching the scan to the map built so far
//        (the front end of an online SLAM system; no loop closure here).
#include "slam.hpp"
#include <cstdio>

struct Run
{
    rb::OccGrid map;
    rb::Pose est;
    double sumSq = 0.0;
    double worst = 0.0;
};

int main()
{
    const rb::Grid truth = rb::makeHouse();
    const rb::LidarSpec lidar;
    const rb::OdoNoise odo;
    rb::Rng rngScan(53);
    rb::Rng rngOdo(530);
    const std::vector<rb::Pose> path = rb::densify(rb::routeWaypoints(), 0.05);

    Run deadReckoning;
    Run slam;
    deadReckoning.est = path[0];     // the start pose defines the map frame
    slam.est = path[0];
    rb::Pose odoPose = path[0];      // pure odometry, integrated step by step
    rb::Pose odoAtLastScan = path[0];
    int scans = 0;
    double sinceScan = 1e9;
    std::printf(" scan  dist(m)  odometry error(m)  slam error(m)  slam heading err(deg)\n");
    double dist = 0.0;
    for (std::size_t k = 0; k < path.size(); ++k) {
        if (k > 0) {
            const rb::Delta d = rb::between(path[k - 1], path[k]);
            const rb::Delta measured = rb::noisy(d, odo, rngOdo);   // what the wheels report
            odoPose = rb::compose(odoPose, measured.fwd, 0.0, measured.dth);
            const double step = std::hypot(d.fwd, d.left);
            sinceScan += step;
            dist += step;
        }
        if (sinceScan < 0.25) { continue; }
        sinceScan = 0.0;
        const std::vector<double> z = rb::scan(truth, path[k], lidar, rngScan);

        // Run A: trust odometry completely.
        deadReckoning.est = odoPose;
        deadReckoning.map.integrate(deadReckoning.est, z, lidar.maxRange);

        // Run B: odometry motion since the last scan, applied to the corrected estimate,
        // then a scan-to-map match (skipped for the very first scan: the map is empty).
        const rb::Delta moved = rb::between(odoAtLastScan, odoPose);
        odoAtLastScan = odoPose;
        rb::Pose guess = rb::compose(slam.est, moved.fwd, moved.left, moved.dth);
        if (scans > 0) { guess = rb::match(slam.map, guess, z, lidar.maxRange).pose; }
        slam.est = guess;
        slam.map.integrate(slam.est, z, lidar.maxRange);

        for (Run* r : {&deadReckoning, &slam}) {
            const double e = std::hypot(r->est.x - path[k].x, r->est.y - path[k].y);
            r->sumSq += e * e;
            r->worst = std::fmax(r->worst, e);
        }
        if (scans % 12 == 0) {
            std::printf("%5d  %7.2f  %17.3f  %13.3f  %21.2f\n", scans, dist,
                        std::hypot(deadReckoning.est.x - path[k].x, deadReckoning.est.y - path[k].y),
                        std::hypot(slam.est.x - path[k].x, slam.est.y - path[k].y),
                        rb::wrapAngle(slam.est.th - path[k].th) * 180.0 / rb::kPi);
        }
        ++scans;
    }
    const char* names[2] = {"odometry only", "scan matching"};
    const Run* runs[2] = {&deadReckoning, &slam};
    for (int r = 0; r < 2; ++r) {
        const rb::Score s = rb::compare(runs[r]->map, truth);
        std::printf("%-14s: RMS position error %.3f m, worst %.3f m; map: wrong 'O' %d"
                    " (far from walls %d), wrong '.' %d\n", names[r], std::sqrt(runs[r]->sumSq / scans),
                    runs[r]->worst, s.falseOcc, s.falseOccFar, s.falseFree);
    }
    std::printf("map from odometry only:\n");
    rb::printMap(deadReckoning.map, true);
    std::printf("map from scan matching:\n");
    rb::printMap(slam.map, true);
    return 0;
}
