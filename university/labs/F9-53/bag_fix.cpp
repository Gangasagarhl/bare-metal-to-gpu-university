// bag_fix.cpp - F9-53 forensic answer key: measure the scan time offset from the data.
// 1. For every pair of consecutive scans, estimate how far the robot turned by finding
//    the circular beam shift that best aligns the two range arrays (sub-beam by a parabola).
// 2. Compare that LiDAR turn rate with the odometry turn rate for a range of assumed
//    offsets; the offset with the smallest mismatch is the stamp error.
// 3. Rebuild the map with the stamps corrected.
#include "bag.hpp"
#include <cstdio>

double beamShift(const std::vector<double>& a, const std::vector<double>& b, double maxRange)
{
    const int n = static_cast<int>(a.size());
    auto cost = [&](int d) {
        double c = 0.0;
        int used = 0;
        for (int k = 0; k < n; ++k) {
            const double rb2 = b[((k - d) % n + n) % n];
            if (a[k] >= maxRange || rb2 >= maxRange) { continue; }
            c += (rb2 - a[k]) * (rb2 - a[k]);
            ++used;
        }
        return used > 0 ? c / used : 1e9;
    };
    int best = 0;
    for (int d = -15; d <= 15; ++d) {
        if (cost(d) < cost(best)) { best = d; }
    }
    const double cm = cost(best - 1);
    const double c0 = cost(best);
    const double cp = cost(best + 1);
    const double denom = cm - 2.0 * c0 + cp;
    return best + (denom > 0 ? 0.5 * (cm - cp) / denom : 0.0);
}

int main()
{
    const rb::Grid world = rb::makeHouse();
    const rb::LidarSpec lidar;
    const rb::Bag bag = rb::record(world, lidar, 0.15);
    const double radPerBeam = 2.0 * rb::kPi / lidar.beams;

    std::vector<double> mid;
    std::vector<double> rate;
    for (std::size_t k = 0; k + 1 < bag.scans.size(); ++k) {
        const rb::ScanMsg& s0 = bag.scans[k];
        const rb::ScanMsg& s1 = bag.scans[k + 1];
        const double dth = beamShift(s0.z, s1.z, lidar.maxRange) * radPerBeam;
        mid.push_back(0.5 * (s0.stamp + s1.stamp));
        rate.push_back(dth / (s1.stamp - s0.stamp));
    }
    std::printf("assumed offset(s)  RMS mismatch of turn rates (rad/s)\n");
    double best = 0.0;
    double bestErr = 1e9;
    for (int k = -10; k <= 30; ++k) {
        const double off = 0.01 * k;
        double sum = 0.0;
        for (std::size_t i = 0; i < mid.size(); ++i) {
            const double e = rate[i] - rb::turnRate(bag, mid[i] - off);
            sum += e * e;
        }
        const double rms = std::sqrt(sum / static_cast<double>(mid.size()));
        if (k % 5 == 0 || (k >= 13 && k <= 17)) { std::printf("  %+.2f             %.4f\n", off, rms); }
        if (rms < bestErr) { bestErr = rms; best = off; }
    }
    std::printf("best offset %+.2f s: scan stamps are that much later than the measurement\n", best);
    const rb::OccGrid fixedMap = rb::mapFromBag(bag, lidar.maxRange, best, false);
    const rb::Score s = rb::compare(fixedMap, world);
    std::printf("map with corrected stamps: wrong 'O' %d (far from walls %d)\n", s.falseOcc, s.falseOccFar);
    rb::printMap(fixedMap, true);
    return 0;
}
