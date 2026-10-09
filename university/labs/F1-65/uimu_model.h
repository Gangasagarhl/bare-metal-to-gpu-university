// F1-65: "U-IMU6", the university's PRETEND six-axis IMU and a pretend I2C bus.
// This is not a real part. Its address, register map, ranges and noise levels are
// invented for teaching; a real IMU's values come only from its datasheet.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <string>
#include <vector>

// A tiny deterministic random generator (so every run prints the same numbers)
// with a Box-Muller transform for Gaussian noise.
class Noise
{
public:
    explicit Noise(std::uint32_t seed) : state_(seed) {}
    double uniform()
    {
        state_ = state_ * 1664525u + 1013904223u;
        return (static_cast<double>(state_ >> 8) + 0.5) / 16777216.0;
    }
    double gaussian(double sigma)
    {
        const double u1 = uniform();
        const double u2 = uniform();
        return sigma * std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::numbers::pi * u2);
    }

private:
    std::uint32_t state_;
};

// Pretend register map of U-IMU6.
namespace reg {
constexpr std::uint8_t kId = 0x00;      // reads 0xA5
constexpr std::uint8_t kConfig = 0x10;  // bits 1..0 accel range, bits 3..2 gyro range
constexpr std::uint8_t kData = 0x20;    // 12 bytes: AX AY AZ GX GY GZ, high byte first
}  // namespace reg

constexpr std::uint8_t kAddress = 0x2A;           // pretend 7-bit I2C address
constexpr std::array<double, 3> kAccelRangeG = {2.0, 4.0, 8.0};
constexpr std::array<double, 3> kGyroRangeDps = {250.0, 500.0, 1000.0};

// The "physical world" the pretend sensor sits in: tilted, not moving.
struct World
{
    double rollDeg = 30.0;
    double pitchDeg = -10.0;
    std::array<double, 3> gyroBiasDps = {0.80, -0.35, 0.10};
    double accelNoiseG = 0.02;
    double gyroNoiseDps = 0.05;
};

class UImu6
{
public:
    UImu6(const World& w, std::uint32_t seed) : world_(w), noise_(seed) {}

    // Register write and read, as the I2C bus delivers them.
    void write(std::uint8_t r, std::uint8_t v)
    {
        if (r == reg::kConfig) {
            config_ = v;
        }
    }
    std::uint8_t read(std::uint8_t r)
    {
        if (r == reg::kId) {
            return 0xA5;
        }
        if (r == reg::kConfig) {
            return config_;
        }
        if (r >= reg::kData && r < reg::kData + 12) {
            if (r == reg::kData) {
                sample();  // reading the first data byte latches a new sample
            }
            return latched_[r - reg::kData];
        }
        return 0x00;
    }

private:
    static std::int16_t toCounts(double value, double range)
    {
        double c = std::round(value / range * 32768.0);
        c = c > 32767.0 ? 32767.0 : (c < -32768.0 ? -32768.0 : c);
        return static_cast<std::int16_t>(c);
    }
    void put(int i, std::int16_t v)
    {
        const auto u = static_cast<std::uint16_t>(v);
        latched_[2 * i] = static_cast<std::uint8_t>(u >> 8);
        latched_[2 * i + 1] = static_cast<std::uint8_t>(u & 0xFF);
    }
    void sample()
    {
        const double r = world_.rollDeg * std::numbers::pi / 180.0;
        const double p = world_.pitchDeg * std::numbers::pi / 180.0;
        // Specific force (in g) seen by a level-z-up accelerometer at rest.
        const std::array<double, 3> a = {-std::sin(p), std::sin(r) * std::cos(p),
                                         std::cos(r) * std::cos(p)};
        const double ar = kAccelRangeG[config_ & 0x3];
        const double gr = kGyroRangeDps[(config_ >> 2) & 0x3];
        for (int i = 0; i < 3; ++i) {
            put(i, toCounts(a[i] + noise_.gaussian(world_.accelNoiseG), ar));
            put(3 + i, toCounts(world_.gyroBiasDps[i] + noise_.gaussian(world_.gyroNoiseDps), gr));
        }
    }

    World world_;
    Noise noise_;
    std::uint8_t config_ = 0x00;
    std::array<std::uint8_t, 12> latched_{};
};

// A pretend I2C bus with one device. It records each transaction as text in the
// order the bus would carry it: S = START, Sr = repeated START, P = STOP,
// A = acknowledge, N = not acknowledge.
class I2cBus
{
public:
    explicit I2cBus(UImu6& dev) : dev_(dev) {}
    bool writeReg(std::uint8_t addr, std::uint8_t r, std::uint8_t v)
    {
        if (addr != kAddress) {
            log("S " + hex(addr) + "+W N P");
            return false;
        }
        dev_.write(r, v);
        log("S " + hex(addr) + "+W A " + hex(r) + " A " + hex(v) + " A P");
        return true;
    }
    bool readRegs(std::uint8_t addr, std::uint8_t r, std::vector<std::uint8_t>& out, int n)
    {
        if (addr != kAddress) {
            log("S " + hex(addr) + "+W N P");
            return false;
        }
        std::string t = "S " + hex(addr) + "+W A " + hex(r) + " A Sr " + hex(addr) + "+R A";
        out.clear();
        for (int i = 0; i < n; ++i) {
            out.push_back(dev_.read(static_cast<std::uint8_t>(r + i)));
            t += " " + hex(out.back()) + (i + 1 < n ? " A" : " N");
        }
        log(t + " P");
        return true;
    }
    bool tracing = true;

private:
    static std::string hex(std::uint8_t v)
    {
        char b[8];
        std::snprintf(b, sizeof b, "%02X", v);
        return b;
    }
    void log(const std::string& s)
    {
        if (tracing) {
            std::printf("  i2c: %s\n", s.c_str());
        }
    }
    UImu6& dev_;
};
