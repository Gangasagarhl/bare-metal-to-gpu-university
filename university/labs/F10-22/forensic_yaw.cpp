// forensic_yaw.cpp - evidence for the F10-22 forensic lab.
// A simulated vehicle turns; a gyro model publishes yaw-rate increments at 1 kHz on a
// uorb_lite topic; the heading estimator reads that topic. The program prints what a
// flight log and a topic-status listing would show.
#include <cstdint>
#include <cstdio>

#include "uorb_lite.h"

namespace {

struct GyroDelta
{
    std::uint64_t timestampUs;
    float dYawDeg;   // yaw change during the last 1 ms
};

struct Heading
{
    std::uint64_t timestampUs;
    float yawDeg;
};

// Yaw rate of the simulated turn, deg/s: still, a 2 s turn at 45 deg/s, still.
double yawRate(double tS)
{
    return (tS >= 1.0 && tS < 3.0) ? 45.0 : 0.0;
}

} // namespace

int main()
{
    uorb_lite::Topic<GyroDelta, 1> gyro("gyro_delta");
    uorb_lite::Topic<Heading, 1> heading("heading");
    uorb_lite::Subscription<GyroDelta, 1> estIn(gyro);
    uorb_lite::Subscription<Heading, 1> logIn(heading);

    double trueYaw = 0.0;
    double estYaw = 0.0;
    std::uint64_t estimatorRuns = 0;
    std::printf("time_s  true_yaw_deg  est_yaw_deg\n");
    for (std::uint64_t ms = 1; ms <= 4000; ++ms) {
        const double t = static_cast<double>(ms) / 1000.0;
        const double d = yawRate(t) / 1000.0;
        trueYaw += d;
        gyro.publish(GyroDelta{ms * 1000, static_cast<float>(d)});
        if (ms % 4 == 0) {   // the estimator's work item runs every 4 ms (250 Hz)
            ++estimatorRuns;
            GyroDelta g{};
            if (estIn.update(g)) {
                estYaw += g.dYawDeg;
            }
            heading.publish(Heading{ms * 1000, static_cast<float>(estYaw)});
        }
        if (ms % 500 == 0) {
            Heading h{};
            while (logIn.update(h)) {
            }
            std::printf("%6.2f  %12.2f  %11.2f\n", t, trueYaw, static_cast<double>(h.yawDeg));
        }
    }
    std::printf("\ntopic status at the end\n");
    std::printf("%-12s %8s %12s %12s %15s\n", "topic", "instance", "queue_len", "published", "reader_lost");
    std::printf("%-12s %8d %12zu %12llu %15llu\n", gyro.name().c_str(), gyro.instance(), gyro.queueLength(),
                static_cast<unsigned long long>(gyro.generation()),
                static_cast<unsigned long long>(estIn.lost()));
    std::printf("%-12s %8d %12zu %12llu %15llu\n", heading.name().c_str(), heading.instance(),
                heading.queueLength(), static_cast<unsigned long long>(heading.generation()),
                static_cast<unsigned long long>(logIn.lost()));
    std::printf("estimator runs: %llu\n", static_cast<unsigned long long>(estimatorRuns));
    return 0;
}
