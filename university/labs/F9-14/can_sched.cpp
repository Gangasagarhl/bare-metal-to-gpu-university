// F9-14 forensic generator: the robot's CAN bus before and after an IMU board was added.
// Each message is sent periodically by its node; the bus model arbitrates by identifier.
// For every identifier the log shows frames queued, frames sent, worst latency (queued ->
// fully received), and how often the receiver saw a gap longer than its timeout.
// Bit rate, identifiers, rates and timeouts are PRETEND values; the fault is in the key.
#include <cstdio>
#include <vector>

#include "can_model.h"

struct Msg
{
    const char* name;
    std::uint32_t id;
    int bytes;
    long periodUs;
    long timeoutUs;  // 0 = receiver does not check
    // statistics
    long queued = 0, sent = 0, worstUs = 0, timeouts = 0;
    long pendingSinceUs = -1;  // -1 = nothing waiting (a new frame overwrites a waiting one)
    long lastRxUs = 0;
};

void simulate(const char* title, std::vector<Msg> msgs)
{
    const long bitrate = 500000;  // bit/s
    const double usPerBit = 1e6 / bitrate;
    const long endUs = 2000000;   // 2 s
    long busFreeUs = 0;
    long busyBits = 0;
    for (long now = 0; now < endUs; ++now) {
        for (Msg& m : msgs) {
            if (now % m.periodUs == 0) {
                ++m.queued;
                if (m.pendingSinceUs < 0) m.pendingSinceUs = now;  // else: overwritten, older lost
            }
            if (m.timeoutUs > 0 && now - m.lastRxUs > m.timeoutUs) {
                ++m.timeouts;
                m.lastRxUs = now;  // the receiver reports once per timeout period
            }
        }
        if (now < busFreeUs) continue;
        Msg* winner = nullptr;
        for (Msg& m : msgs) {
            if (m.pendingSinceUs >= 0 && (winner == nullptr || m.id < winner->id)) winner = &m;
        }
        if (winner == nullptr) continue;
        const int bits = canFrameBitsWorst(winner->bytes);
        busyBits += bits;
        busFreeUs = now + static_cast<long>(bits * usPerBit);
        const long latency = busFreeUs - winner->pendingSinceUs;
        if (latency > winner->worstUs) winner->worstUs = latency;
        ++winner->sent;
        winner->lastRxUs = busFreeUs;
        winner->pendingSinceUs = -1;
    }
    std::printf("%s\n", title);
    std::printf("  bus busy %.1f %% of the time (2 s at %ld bit/s, worst-case frames of %d bits)\n",
                100.0 * busyBits / (endUs / usPerBit), bitrate, canFrameBitsWorst(8));
    std::printf("  %-14s %6s %8s %7s %6s %14s %9s\n", "message", "id", "rate_Hz", "queued", "sent",
                "worst_lat_us", "timeouts");
    for (const Msg& m : msgs) {
        std::printf("  %-14s 0x%03X %8ld %7ld %6ld %14ld %9ld\n", m.name, m.id & CAN_SFF_MASK,
                    1000000 / m.periodUs, m.queued, m.sent, m.worstUs, m.timeouts);
    }
    std::printf("\n");
}

int main()
{
    std::vector<Msg> base{
        {"estop_beat", 0x080, 1, 20000, 100000},
        {"status_L", 0x180, 8, 10000, 0},
        {"status_R", 0x181, 8, 10000, 0},
        {"odom_L", 0x190, 8, 10000, 0},
        {"odom_R", 0x191, 8, 10000, 0},
        {"wheel_cmd", 0x300, 8, 10000, 30000},
        {"battery", 0x700, 8, 100000, 0},
    };
    simulate("BEFORE (Monday build)", base);
    std::vector<Msg> after = base;
    after.push_back({"imu_accel", 0x100, 8, 1000, 0});
    after.push_back({"imu_gyro", 0x101, 8, 1000, 0});
    after.push_back({"imu_mag_temp", 0x102, 8, 1000, 0});
    after.push_back({"imu_status", 0x103, 8, 1000, 0});
    simulate("AFTER (IMU board added, Wednesday build)", after);
    return 0;
}
