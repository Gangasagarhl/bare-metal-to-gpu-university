// uorb_demo.cpp - single-threaded, deterministic experiments with uorb_lite.h.
// A gyro "driver" publishes one message per millisecond. Subscribers read in
// different ways: polling every 4 ms with a queue of 1, polling with a queue of 4,
// and being woken by a callback on every publication.
#include <cstdint>
#include <cstdio>

#include "uorb_lite.h"

namespace {

struct GyroSample
{
    std::uint64_t timestampUs;
    float deltaAngleDeg;   // rotation during the last 1 ms
};

template <std::size_t Q>
void pollingSubscriber(const char* label)
{
    uorb_lite::Topic<GyroSample, Q> gyro("sensor_gyro_model");
    uorb_lite::Subscription<GyroSample, Q> sub(gyro);
    int reads = 0;
    double angle = 0.0;
    for (std::uint64_t t = 1; t <= 1000; ++t) {             // 1 s of 1 kHz samples
        gyro.publish(GyroSample{t * 1000, 0.09f});           // 90 deg/s
        if (t % 4 == 0) {                                     // the subscriber runs every 4 ms
            GyroSample s{};
            while (sub.update(s)) {
                angle += s.deltaAngleDeg;
                ++reads;
            }
        }
    }
    std::printf("%-34s published 1000, read %4d, lost %4llu, integrated angle %.2f deg\n", label, reads,
                static_cast<unsigned long long>(sub.lost()), angle);
}

} // namespace

int main()
{
    std::printf("1) polling every 4 ms, true rotation 90.00 deg\n");
    pollingSubscriber<1>("queue length 1:");
    pollingSubscriber<4>("queue length 4:");

    std::printf("\n2) woken on every publication (callback), queue length 1\n");
    uorb_lite::Topic<GyroSample, 1> gyro("sensor_gyro_model");
    uorb_lite::Subscription<GyroSample, 1> sub(gyro);
    int reads = 0;
    double angle = 0.0;
    gyro.onPublish([&] {
        GyroSample s{};
        while (sub.update(s)) {
            angle += s.deltaAngleDeg;
            ++reads;
        }
    });
    for (std::uint64_t t = 1; t <= 1000; ++t) {
        gyro.publish(GyroSample{t * 1000, 0.09f});
    }
    std::printf("%-34s published 1000, read %4d, lost %4llu, integrated angle %.2f deg\n", "callback:", reads,
                static_cast<unsigned long long>(sub.lost()), angle);

    std::printf("\n3) a late subscriber and two instances of one topic\n");
    uorb_lite::Topic<GyroSample, 1> gpsA("vehicle_gps_model", 0);
    uorb_lite::Topic<GyroSample, 1> gpsB("vehicle_gps_model", 1);
    gpsA.publish(GyroSample{100, 1.0f});
    gpsA.publish(GyroSample{200, 2.0f});
    uorb_lite::Subscription<GyroSample, 1> late(gpsA);   // created after two publications
    uorb_lite::Subscription<GyroSample, 1> other(gpsB);  // instance 1 has never been published
    GyroSample s{};
    const bool got = late.update(s);
    std::printf("late subscriber on %s instance %d: updated=%s, value t=%llu\n", gpsA.name().c_str(),
                gpsA.instance(), got ? "yes" : "no", static_cast<unsigned long long>(s.timestampUs));
    std::printf("subscriber on %s instance %d: updated()=%s\n", gpsB.name().c_str(), gpsB.instance(),
                other.updated() ? "yes" : "no");
    return 0;
}
