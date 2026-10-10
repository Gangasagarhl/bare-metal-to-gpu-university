// glitch.cpp - F10-15 forensic evidence generator "The 12-metre lurch".
// The F10-13 recording is replayed through the EKF of ins.hpp with a GNSS fault added:
// between 19.0 s and 21.0 s the receiver reports positions shifted 12 m along x
// (velocity unchanged). Two estimator logs are printed: the vehicle's own configuration
// and the university's default. The configuration difference is shown in each header.
#include "ins.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

struct GnssRow
{
    double t;
    std::array<double, 6> z;
    std::array<double, 6> truth;
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

void replay(const std::vector<imu::Row>& imuRows, std::vector<GnssRow> gnss, double gate,
            const char* title)
{
    for (GnssRow& g : gnss) {
        if (g.t > 18.95 && g.t < 21.05) {
            g.z[0] += 12.0; // the fault: position jumps 12 m along x
        }
    }
    ins::Filter f;
    f.gate = gate;
    std::printf("%s\n", title);
    std::printf("config: gnss_gate_sigma = %.0f\n", gate);
    std::printf("  t(s) | gnss px | ekf px | innov px | ratio px | fused | ekf vx\n");
    std::size_t g = 0;
    double tPrev = 0.0;
    for (const imu::Row& r : imuRows) {
        f.predict({r.gx, r.gy, r.gz, r.ax, r.ay, r.az}, r.t - tPrev);
        tPrev = r.t;
        if (g < gnss.size() && std::fabs(gnss[g].t - r.t) < 1e-6) {
            const bool fused = f.updateGnss(gnss[g].z);
            const long tenth = std::lround(r.t * 10.0);
            if (tenth >= 186 && tenth <= 226 && tenth % 2 == 0) {
                std::printf("%6.1f | %7.2f | %6.2f | %8.2f | %8.1f | %5s | %6.2f\n", r.t,
                            gnss[g].z[0], f.x[6], f.lastInnovation[0], f.lastRatio[0],
                            fused ? "yes" : "no", f.x[3]);
            }
            ++g;
        }
    }
    std::printf("GNSS samples fused %ld, rejected %ld\n\n", f.accepted, f.rejected);
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
    replay(imuRows, gnss, 100.0, "Estimator log A: the club vehicle's configuration");
    replay(imuRows, gnss, 5.0,
           "Estimator log B: same flight data, university default configuration");
    return 0;
}
