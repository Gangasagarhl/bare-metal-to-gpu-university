// ekf_ins.cpp - runs the GNSS-aided EKF of ins.hpp on the F10-13 recording
// (imu_flight.csv at 100 Hz, gnss_flight.csv at 10 Hz) and grades it against the truth.
#include "ins.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

struct GnssRow
{
    double t;
    std::array<double, 6> z;     // measured px py pz vx vy vz
    std::array<double, 6> truth; // true px py pz vx vy vz
};

std::vector<GnssRow> readGnss(const char* path)
{
    std::vector<GnssRow> rows;
    std::FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        return rows;
    }
    char header[256];
    if (std::fgets(header, sizeof header, f) != nullptr) {
        GnssRow r{};
        while (std::fscanf(f, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &r.t, &r.z[0],
                           &r.z[1], &r.z[2], &r.z[3], &r.z[4], &r.z[5], &r.truth[0], &r.truth[1],
                           &r.truth[2], &r.truth[3], &r.truth[4], &r.truth[5]) == 13) {
            rows.push_back(r);
        }
    }
    std::fclose(f);
    return rows;
}

} // namespace

int main()
{
    const auto imuRows = imu::readCsv("../F10-13/imu_flight.csv");
    const auto gnss = readGnss("../F10-13/gnss_flight.csv");
    if (imuRows.empty() || gnss.empty()) {
        std::printf("cannot read the F10-13 recording\n");
        return 1;
    }
    ins::Filter f;
    std::size_t g = 0;
    double tPrev = 0.0;
    std::array<double, 6> tiltSq{};
    std::array<int, 6> tiltN{};
    const std::array<double, 7> edge{0.0, 2.0, 6.0, 13.0, 17.0, 21.0, 25.1};
    double yawSq = 0.0, posSq = 0.0, gnssPosSq = 0.0;
    int posN = 0;
    std::printf("  t(s) | roll err pitch err yaw err (deg) | pos err (m) | max ratio\n");
    for (const imu::Row& r : imuRows) {
        f.predict({r.gx, r.gy, r.gz, r.ax, r.ay, r.az}, r.t - tPrev);
        tPrev = r.t;
        double ratio = 0.0;
        if (g < gnss.size() && std::fabs(gnss[g].t - r.t) < 1e-6) {
            f.updateGnss(gnss[g].z);
            for (double v : f.lastRatio) {
                ratio = std::fmax(ratio, v);
            }
            const auto& tr = gnss[g].truth;
            const double ep = std::hypot(f.x[6] - tr[0], f.x[7] - tr[1], f.x[8] - tr[2]);
            posSq += ep * ep;
            const double eg =
                std::hypot(gnss[g].z[0] - tr[0], gnss[g].z[1] - tr[1], gnss[g].z[2] - tr[2]);
            gnssPosSq += eg * eg;
            ++posN;
            if (posN % 20 == 0) {
                std::printf("%6.1f | %8.2f %9.2f %7.2f      | %7.3f     | %5.2f\n", r.t,
                            f.x[0] / imu::kDeg - r.roll, f.x[1] / imu::kDeg - r.pitch,
                            imu::wrapPi(f.x[2] - r.yaw * imu::kDeg) / imu::kDeg, ep, ratio);
            }
            ++g;
        }
        const double er = f.x[0] / imu::kDeg - r.roll, ep = f.x[1] / imu::kDeg - r.pitch;
        for (int i = 0; i < 6; ++i) {
            if (r.t > edge[i] && r.t <= edge[i + 1]) {
                tiltSq[i] += er * er + ep * ep;
                ++tiltN[i];
            }
        }
        const double ey = imu::wrapPi(f.x[2] - r.yaw * imu::kDeg) / imu::kDeg;
        yawSq += ey * ey;
    }
    std::printf("\nRMS tilt error (deg) per part 1-6:");
    double all = 0.0;
    int count = 0;
    for (int i = 0; i < 6; ++i) {
        std::printf(" %5.2f", std::sqrt(tiltSq[i] / tiltN[i]));
        all += tiltSq[i];
        count += tiltN[i];
    }
    std::printf(" | all %5.2f\n", std::sqrt(all / count));
    std::printf("RMS yaw error: %.2f deg\n",
                std::sqrt(yawSq / static_cast<double>(imuRows.size())));
    std::printf("RMS position error: EKF %.3f m, raw GNSS %.3f m (%d GNSS samples, %ld fused, %ld "
                "rejected)\n",
                std::sqrt(posSq / posN), std::sqrt(gnssPosSq / posN), posN, f.accepted, f.rejected);
    std::printf("gyro bias estimate (rad/s): %.4f %.4f %.4f (true 0.0040 -0.0060 0.0030)\n", f.x[9],
                f.x[10], f.x[11]);
    std::printf("1-sigma from P: roll %.2f deg, yaw %.2f deg, bx %.4f rad/s\n",
                std::sqrt(f.P(0, 0)) / imu::kDeg, std::sqrt(f.P(2, 2)) / imu::kDeg,
                std::sqrt(f.P(9, 9)));
    return 0;
}
