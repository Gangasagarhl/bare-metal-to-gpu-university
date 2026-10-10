// F1-65 Listing 1: a driver for the pretend U-IMU6 (uimu_model.h).
// Probe, configure, read raw bytes, convert to units, calibrate the gyro bias,
// and compute tilt from the accelerometer. Every part number and register here is
// the university's invention; a real driver follows the real part's datasheet.
#include "uimu_model.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <vector>

struct Reading
{
    double ax, ay, az;  // g
    double gx, gy, gz;  // degrees per second
};

std::int16_t be16(const std::vector<std::uint8_t>& b, int i)
{
    const auto u = static_cast<std::uint16_t>((b[2 * i] << 8) | b[2 * i + 1]);
    return static_cast<std::int16_t>(u);  // two's complement, high byte first
}

bool readImu(I2cBus& bus, double accelRange, double gyroRange, Reading& r)
{
    std::vector<std::uint8_t> b;
    if (!bus.readRegs(kAddress, reg::kData, b, 12)) {
        return false;
    }
    const double as = accelRange / 32768.0;  // g per count
    const double gs = gyroRange / 32768.0;   // dps per count
    r = {be16(b, 0) * as, be16(b, 1) * as, be16(b, 2) * as,
         be16(b, 3) * gs, be16(b, 4) * gs, be16(b, 5) * gs};
    return true;
}

int main()
{
    World world;
    UImu6 imu(world, 12345u);
    I2cBus bus(imu);

    std::printf("1. Probe: read the ID register\n");
    std::vector<std::uint8_t> id;
    bus.readRegs(kAddress, reg::kId, id, 1);
    if (id.empty() || id[0] != 0xA5) {
        std::printf("   wrong or missing device\n");
        return 1;
    }
    std::printf("   ID = 0x%02X (expected 0xA5)\n", id[0]);

    std::printf("2. Configure: accel range index 1 (+-4 g), gyro range index 1 (+-500 dps)\n");
    const std::uint8_t config = (1u << 2) | 1u;
    bus.writeReg(kAddress, reg::kConfig, config);
    const double ar = kAccelRangeG[config & 0x3];
    const double gr = kGyroRangeDps[(config >> 2) & 0x3];

    std::printf("3. One raw read of 12 data bytes\n");
    std::vector<std::uint8_t> raw;
    bus.readRegs(kAddress, reg::kData, raw, 12);
    const std::int16_t azCounts = be16(raw, 2);
    const auto wrongOrder = static_cast<std::int16_t>((raw[5] << 8) | raw[4]);
    std::printf("   AZ bytes %02X %02X -> %d counts -> %.4f g\n", raw[4], raw[5], azCounts,
                azCounts * ar / 32768.0);
    std::printf("   (bytes swapped by mistake -> %d counts -> %.4f g)\n", wrongOrder,
                wrongOrder * ar / 32768.0);

    std::printf("4. Calibrate gyro bias: average 200 samples while not moving\n");
    bus.tracing = false;
    double sum[6] = {0, 0, 0, 0, 0, 0};
    const int n = 200;
    for (int k = 0; k < n; ++k) {
        Reading r{};
        if (!readImu(bus, ar, gr, r)) {
            return 1;
        }
        const double v[6] = {r.ax, r.ay, r.az, r.gx, r.gy, r.gz};
        for (int i = 0; i < 6; ++i) {
            sum[i] += v[i];
        }
    }
    double mean[6];
    for (int i = 0; i < 6; ++i) {
        mean[i] = sum[i] / n;
    }
    std::printf("   gyro bias estimate: %+.3f %+.3f %+.3f dps (model truth %+.2f %+.2f %+.2f)\n",
                mean[3], mean[4], mean[5], world.gyroBiasDps[0], world.gyroBiasDps[1],
                world.gyroBiasDps[2]);

    std::printf("5. Tilt from the averaged accelerometer\n");
    const double rad = 180.0 / std::numbers::pi;
    const double roll = std::atan2(mean[1], mean[2]) * rad;
    const double yz = std::sqrt(mean[1] * mean[1] + mean[2] * mean[2]);
    const double pitch = std::atan2(-mean[0], yz) * rad;
    const double norm = std::sqrt(mean[0] * mean[0] + mean[1] * mean[1] + mean[2] * mean[2]);
    std::printf("   accel mean: %+.4f %+.4f %+.4f g, length %.4f g\n", mean[0], mean[1], mean[2],
                norm);
    std::printf("   roll %.2f deg (truth %.1f), pitch %.2f deg (truth %.1f)\n", roll,
                world.rollDeg, pitch, world.pitchDeg);

    std::printf("6. A read from the wrong address\n");
    bus.tracing = true;
    std::vector<std::uint8_t> none;
    if (!bus.readRegs(0x2B, reg::kId, none, 1)) {
        std::printf("   no acknowledge: report an error, do not hang\n");
    }
    return 0;
}
