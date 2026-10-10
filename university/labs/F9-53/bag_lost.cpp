// bag_lost.cpp - F9-53 forensic evidence pack: "the robot that is lost".
// Records the bag (with the fault the answer key describes), then prints what the
// team's tools print: a bag summary, an excerpt during a turn, and two maps.
#include "bag.hpp"
#include <cstdio>

int main()
{
    const rb::Grid world = rb::makeHouse();
    const rb::LidarSpec lidar;
    const rb::Bag bag = rb::record(world, lidar, 0.15);

    auto delay = [](const auto& msgs) {
        double sum = 0.0;
        for (const auto& m : msgs) { sum += m.recv - m.stamp; }
        return sum / static_cast<double>(msgs.size());
    };
    std::printf("bag summary\n");
    std::printf("  /odom  %5zu msgs  stamps %.3f .. %.3f s  mean (receive - stamp) %.3f s\n",
                bag.odom.size(), bag.odom.front().stamp, bag.odom.back().stamp, delay(bag.odom));
    std::printf("  /scan  %5zu msgs  stamps %.3f .. %.3f s  mean (receive - stamp) %.3f s\n",
                bag.scans.size(), bag.scans.front().stamp, bag.scans.back().stamp, delay(bag.scans));

    std::printf("excerpt (receive order) while odometry reports a turn:\n");
    std::printf("  recv(s)   topic  stamp(s)  content\n");
    const double t0 = 33.0;
    std::size_t o = 0;
    std::size_t s = 0;
    while (o < bag.odom.size() || s < bag.scans.size()) {
        const bool takeScan = s < bag.scans.size() &&
                              (o >= bag.odom.size() || bag.scans[s].recv < bag.odom[o].recv);
        const double recv = takeScan ? bag.scans[s].recv : bag.odom[o].recv;
        if (recv >= t0 && recv < t0 + 0.25) {
            if (takeScan) {
                std::printf("  %7.3f  /scan  %7.3f  360 ranges, beam 0 = %.3f m\n", recv,
                            bag.scans[s].stamp, bag.scans[s].z[0]);
            } else if (o % 2 == 0) {
                std::printf("  %7.3f  /odom  %7.3f  x %.3f y %.3f th %+.3f rad\n", recv, bag.odom[o].stamp,
                            bag.odom[o].pose.x, bag.odom[o].pose.y, bag.odom[o].pose.th);
            }
        }
        if (takeScan) { ++s; } else { ++o; }
    }
    const rb::OccGrid all = rb::mapFromBag(bag, lidar.maxRange, 0.0, false);
    const rb::OccGrid straight = rb::mapFromBag(bag, lidar.maxRange, 0.0, true);
    const rb::Score sa = rb::compare(all, world);
    const rb::Score ss = rb::compare(straight, world);
    std::printf("map from all scans:            wrong 'O' %d (far from walls %d)\n", sa.falseOcc, sa.falseOccFar);
    std::printf("map from scans while straight: wrong 'O' %d (far from walls %d)\n", ss.falseOcc, ss.falseOccFar);
    std::printf("map from all scans:\n");
    rb::printMap(all, true);
    return 0;
}
