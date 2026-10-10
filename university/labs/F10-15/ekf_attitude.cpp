// ekf_attitude.cpp - an extended Kalman filter for roll, pitch and two gyro biases,
// run on the recorded IMU data of F10-13. State x = [roll, pitch, bx, by].
// Predict: integrate the bias-corrected gyro (Euler-angle rates). Update: the accelerometer,
// modelled as gravity only: h(x) = g (-sin pitch, cos pitch sin roll, cos pitch cos roll).
#include "../F10-13/imu.hpp"
#include "mat.hpp"

#include <array>
#include <cmath>
#include <cstdio>

namespace {

using State = std::array<double, 4>;

State predictState(const State& x, const imu::Row& r, double dt)
{
    const double p = r.gx - x[2], q = r.gy - x[3], w = r.gz; // z bias not estimated
    const double sr = std::sin(x[0]), cr = std::cos(x[0]);
    return {x[0] + dt * (p + (q * sr + w * cr) * std::tan(x[1])), x[1] + dt * (q * cr - w * sr),
            x[2], x[3]};
}

Mat jacobianF(const State& x, const imu::Row& r, double dt)
{
    const double q = r.gy - x[3], w = r.gz;
    const double sr = std::sin(x[0]), cr = std::cos(x[0]);
    const double tp = std::tan(x[1]), cp = std::cos(x[1]);
    return Mat(4, 4,
               {1.0 + dt * (q * cr - w * sr) * tp, dt * (q * sr + w * cr) / (cp * cp), -dt,
                -dt * sr * tp,                               //
                dt * (-q * sr - w * cr), 1.0, 0.0, -dt * cr, //
                0.0, 0.0, 1.0, 0.0,                          //
                0.0, 0.0, 0.0, 1.0});
}

std::array<double, 3> measure(const State& x)
{
    const double sr = std::sin(x[0]), cr = std::cos(x[0]);
    const double sp = std::sin(x[1]), cp = std::cos(x[1]);
    return {-imu::kG * sp, imu::kG * cp * sr, imu::kG * cp * cr};
}

Mat jacobianH(const State& x)
{
    const double sr = std::sin(x[0]), cr = std::cos(x[0]);
    const double sp = std::sin(x[1]), cp = std::cos(x[1]);
    const double g = imu::kG;
    return Mat(3, 4,
               {0.0, -g * cp, 0.0, 0.0,              //
                g * cp * cr, -g * sp * sr, 0.0, 0.0, //
                -g * cp * sr, -g * sp * cr, 0.0, 0.0});
}

// Central differences: the largest gap between the coded Jacobians and numerical ones.
double checkJacobians()
{
    const State x{0.3, -0.2, 0.01, -0.02};
    imu::Row r;
    r.gx = 0.5;
    r.gy = -0.4;
    r.gz = 0.7;
    const double dt = 0.01, e = 1e-6;
    const Mat F = jacobianF(x, r, dt), H = jacobianH(x);
    double worst = 0.0;
    for (int j = 0; j < 4; ++j) {
        State a = x, b = x;
        a[j] += e;
        b[j] -= e;
        const State fa = predictState(a, r, dt), fb = predictState(b, r, dt);
        const auto ha = measure(a), hb = measure(b);
        for (int i = 0; i < 4; ++i) {
            worst = std::fmax(worst, std::fabs((fa[i] - fb[i]) / (2 * e) - F(i, j)));
        }
        for (int i = 0; i < 3; ++i) {
            worst = std::fmax(worst, std::fabs((ha[i] - hb[i]) / (2 * e) - H(i, j)));
        }
    }
    return worst;
}

void run(const std::vector<imu::Row>& rows, double accelSigma)
{
    State x{0.0, 0.0, 0.0, 0.0};
    Mat P(4, 4, {0.01, 0, 0, 0, 0, 0.01, 0, 0, 0, 0, 1e-4, 0, 0, 0, 0, 1e-4});
    const double gyroSigma = 0.003, biasWalk = 2e-4; // rad/s; rad/s per sqrt(s)
    const Mat R = (accelSigma * accelSigma) * Mat::identity(3);
    std::array<double, 6> sq{};
    std::array<int, 6> n{};
    const std::array<double, 7> edge{0.0, 2.0, 6.0, 13.0, 17.0, 21.0, 25.1};
    double nisSum = 0.0;
    double tPrev = 0.0;
    for (const imu::Row& r : rows) {
        const double dt = r.t - tPrev;
        tPrev = r.t;
        // Predict
        const Mat F = jacobianF(x, r, dt);
        x = predictState(x, r, dt);
        const double qa = gyroSigma * gyroSigma * dt * dt, qb = biasWalk * biasWalk * dt;
        const Mat Q(4, 4, {qa, 0, 0, 0, 0, qa, 0, 0, 0, 0, qb, 0, 0, 0, 0, qb});
        P = F * P * F.t() + Q;
        // Update with the accelerometer
        const auto hx = measure(x);
        const Mat y(3, 1, {r.ax - hx[0], r.ay - hx[1], r.az - hx[2]});
        const Mat H = jacobianH(x);
        const Mat S = H * P * H.t() + R;
        const Mat Si = inverse(S);
        const Mat K = P * H.t() * Si;
        const Mat dx = K * y;
        for (int i = 0; i < 4; ++i) {
            x[static_cast<std::size_t>(i)] += dx(i, 0);
        }
        const Mat IKH = Mat::identity(4) - K * H;
        P = IKH * P * IKH.t() + K * R * K.t(); // Joseph form
        nisSum += (y.t() * Si * y)(0, 0);
        // Score against the true attitude
        const double er = x[0] / imu::kDeg - r.roll, ep = x[1] / imu::kDeg - r.pitch;
        for (int i = 0; i < 6; ++i) {
            if (r.t > edge[i] && r.t <= edge[i + 1]) {
                sq[i] += er * er + ep * ep;
                ++n[i];
            }
        }
    }
    double all = 0.0;
    int count = 0;
    std::printf("EKF accel sigma %4.2f m/s^2 |", accelSigma);
    for (int i = 0; i < 6; ++i) {
        std::printf(" %5.2f", std::sqrt(sq[i] / n[i]));
        all += sq[i];
        count += n[i];
    }
    std::printf(" | %5.2f | NIS %6.2f | bias %7.4f %7.4f\n", std::sqrt(all / count),
                nisSum / static_cast<double>(rows.size()), x[2], x[3]);
}

} // namespace

int main()
{
    std::printf("Jacobian check (largest |coded - central difference|): %.2e\n", checkJacobians());
    const auto rows = imu::readCsv("../F10-13/imu_flight.csv");
    if (rows.size() < 2) {
        std::printf("cannot read ../F10-13/imu_flight.csv\n");
        return 1;
    }
    std::printf("RMS tilt error (deg) per part: 1 hover, 2 roll doublet, 3 accelerate/brake,\n");
    std::printf("4 yaw turn, 5 diagonal accel, 6 hover again; mean NIS (3 if consistent);\n");
    std::printf("final bias estimates bx, by (rad/s); true bias 0.0040 -0.0060\n");
    std::printf("%27s    1     2     3     4     5     6 |  all\n", "");
    for (double sigma : {0.25, 1.0, 2.0, 4.0}) {
        run(rows, sigma);
    }
    return 0;
}
