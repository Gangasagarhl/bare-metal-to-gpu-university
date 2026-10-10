// F10-08 Listing 2: hard-iron calibration of a magnetometer by fitting a sphere,
// then a tilt-compensated heading. Frames: world x north, y east, z down;
// body x forward, y right, z down. Field, offset and noise are exercise values.
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numbers>
#include <vector>

using Vec = std::array<double, 3>;
constexpr double kDeg = std::numbers::pi / 180.0;

struct Rng  // small deterministic generator so the output never changes
{
    std::uint64_t s = 20261010;
    double uniform()
    {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<double>(s >> 11) * 0x1.0p-53;
    }
    double gauss()
    {
        const double r = std::sqrt(-2.0 * std::log(1.0 - uniform()));
        return r * std::cos(2.0 * std::numbers::pi * uniform());
    }
};

// World vector seen in the body frame for roll phi, pitch theta, yaw psi (R = Rz Ry Rx).
Vec toBody(const Vec& w, double phi, double theta, double psi)
{
    const double cf = std::cos(phi), sf = std::sin(phi);
    const double ct = std::cos(theta), st = std::sin(theta);
    const double cp = std::cos(psi), sp = std::sin(psi);
    const double R[3][3] = {{ct * cp, sf * st * cp - cf * sp, cf * st * cp + sf * sp},
                            {ct * sp, sf * st * sp + cf * cp, cf * st * sp - sf * cp},
                            {-st, sf * ct, cf * ct}};
    Vec b{};
    for (int i = 0; i < 3; ++i) {
        b[i] = R[0][i] * w[0] + R[1][i] * w[1] + R[2][i] * w[2];   // R transposed
    }
    return b;
}

double headingDeg(const Vec& m, double phi, double theta)
{
    const double xh = m[0] * std::cos(theta) + m[1] * std::sin(phi) * std::sin(theta) +
                      m[2] * std::cos(phi) * std::sin(theta);
    const double yh = m[1] * std::cos(phi) - m[2] * std::sin(phi);
    double h = std::atan2(-yh, xh) / kDeg;
    return h < 0 ? h + 360.0 : h;
}

// Sphere fit: |m|^2 = 2 c.m + d, linear least squares in (cx, cy, cz, d).
Vec fitCentre(const std::vector<Vec>& pts)
{
    double A[4][5] = {};
    for (const auto& p : pts) {
        const double row[4] = {2 * p[0], 2 * p[1], 2 * p[2], 1.0};
        const double rhs = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                A[i][j] += row[i] * row[j];
            }
            A[i][4] += row[i] * rhs;
        }
    }
    for (int c = 0; c < 4; ++c) {             // Gauss-Jordan elimination, no pivoting needed here
        for (int r = 0; r < 4; ++r) {
            if (r != c) {
                const double f = A[r][c] / A[c][c];
                for (int k = c; k < 5; ++k) {
                    A[r][k] -= f * A[c][k];
                }
            }
        }
    }
    return {A[0][4] / A[0][0], A[1][4] / A[1][1], A[2][4] / A[2][2]};
}

int main()
{
    const Vec field{20.0, 0.0, 45.0};          // uT, world frame (exercise value)
    const Vec offset{12.0, -7.0, 4.0};         // uT, hard iron fixed to the body (exercise)
    Rng rng;
    std::vector<Vec> samples;
    for (int i = 0; i < 300; ++i) {            // the "calibration dance": many orientations
        const double phi = (rng.uniform() * 2 - 1) * 180 * kDeg;
        const double theta = (rng.uniform() * 2 - 1) * 85 * kDeg;
        const double psi = rng.uniform() * 360 * kDeg;
        Vec m = toBody(field, phi, theta, psi);
        for (int k = 0; k < 3; ++k) {
            m[k] += offset[k] + 0.3 * rng.gauss();
        }
        samples.push_back(m);
    }
    const Vec c = fitCentre(samples);
    std::printf("fitted hard-iron offset: %+.2f %+.2f %+.2f uT (model: %+.1f %+.1f %+.1f)\n",
                c[0], c[1], c[2], offset[0], offset[1], offset[2]);

    std::printf("true yaw  roll pitch | raw heading  error | calibrated  error\n");
    const double cases[5][3] = {{0, 0, 0}, {90, 0, 0}, {200, 0, 0}, {90, 20, -10}, {300, -15, 25}};
    for (const auto& k : cases) {
        const double psi = k[0] * kDeg, phi = k[1] * kDeg, theta = k[2] * kDeg;
        Vec m = toBody(field, phi, theta, psi);
        Vec cal{};
        for (int i = 0; i < 3; ++i) {
            m[i] += offset[i];
            cal[i] = m[i] - c[i];
        }
        const double hr = headingDeg(m, phi, theta), hc = headingDeg(cal, phi, theta);
        auto err = [&](double h) { return std::remainder(h - k[0], 360.0); };
        std::printf("%8.0f %5.0f %5.0f | %10.1f %+6.1f | %10.1f %+6.1f\n", k[0], k[1], k[2], hr,
                    err(hr), hc, err(hc));
    }
    return 0;
}
