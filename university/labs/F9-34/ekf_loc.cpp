// F9-34 Listing 3: EKF localisation with known landmarks. Prediction uses wheel odometry
// (v, w); each update uses one range-bearing reading. The run is compared with ground truth
// and with odometry alone (dead reckoning).
#include "mat.hpp"

#include <fstream>
#include <sstream>
#include <string>

const double kPi = std::acos(-1.0);

double wrap(double a)  // to (-pi, pi]
{
    while (a > kPi) {
        a -= 2.0 * kPi;
    }
    while (a <= -kPi) {
        a += 2.0 * kPi;
    }
    return a;
}

int main()
{
    const double dt = 0.1;
    const double lmx[3] = {4.0, -4.0, 0.0};
    const double lmy[3] = {4.0, 2.0, -5.0};
    const Mat M(2, 2, {0.05 * 0.05, 0.0, 0.0, 0.05 * 0.05});  // odometry noise (v, w)
    const double bSd = 2.0 * kPi / 180.0;
    const Mat R(2, 2, {0.1 * 0.1, 0.0, 0.0, bSd * bSd});        // range, bearing noise

    Mat s(3, 1, {2.8, 0.2, kPi / 2.0 + 0.1});  // first guess, a little wrong on purpose
    Mat P(3, 3, {0.1, 0.0, 0.0, 0.0, 0.1, 0.0, 0.0, 0.0, 0.05});
    double ox = 3.0, oy = 0.0, oth = kPi / 2.0;  // odometry alone, from the true start

    std::ifstream in("ekf_log.csv");
    std::string line;
    std::getline(in, line);
    double sePos = 0.0, seTh = 0.0, nees = 0.0, nis = 0.0;
    double lastX = 0.0, lastY = 0.0;
    int n = 0, k = 0;
    std::cout << "    t   x_est   y_est  th_est  truth_x truth_y truth_th  sd_x   sd_y   sd_th\n";
    while (std::getline(in, line)) {
        std::istringstream f(line);
        double t, v, w, r, b, tx, ty, tth;
        int id;
        char c;
        f >> t >> c >> v >> c >> w >> c >> id >> c >> r >> c >> b >> c >> tx >> c >> ty >> c >> tth;
        ++k;
        lastX = tx;
        lastY = ty;
        // Predict: s = g(s, u); P = G P G^T + V M V^T.
        const double th = s(2, 0);
        const Mat G(3, 3, {1.0, 0.0, -v * dt * std::sin(th),
                           0.0, 1.0, v * dt * std::cos(th),
                           0.0, 0.0, 1.0});
        const Mat V(3, 2, {dt * std::cos(th), 0.0, dt * std::sin(th), 0.0, 0.0, dt});
        s = Mat(3, 1, {s(0, 0) + v * dt * std::cos(th), s(1, 0) + v * dt * std::sin(th),
                       wrap(th + w * dt)});
        P = G * P * G.t() + V * M * V.t();
        ox += v * dt * std::cos(oth);
        oy += v * dt * std::sin(oth);
        oth = wrap(oth + w * dt);

        // Update with the landmark reading: linearise h at the predicted state.
        const double dx = lmx[id] - s(0, 0), dy = lmy[id] - s(1, 0);
        const double q = dx * dx + dy * dy;
        const Mat zhat(2, 1, {std::sqrt(q), wrap(std::atan2(dy, dx) - s(2, 0))});
        const Mat H(2, 3, {-dx / std::sqrt(q), -dy / std::sqrt(q), 0.0, dy / q, -dx / q, -1.0});
        Mat y = Mat(2, 1, {r, b}) - zhat;
        y(1, 0) = wrap(y(1, 0));  // a bearing difference is an angle: wrap it
        const Mat S = H * P * H.t() + R;
        const Mat K = P * H.t() * inverse(S);
        s = s + K * y;
        s(2, 0) = wrap(s(2, 0));
        const Mat IKH = Mat::identity(3) - K * H;
        P = IKH * P * IKH.t() + K * R * K.t();

        const Mat e(3, 1, {s(0, 0) - tx, s(1, 0) - ty, wrap(s(2, 0) - tth)});
        if (k > 20) {
            sePos += e(0, 0) * e(0, 0) + e(1, 0) * e(1, 0);
            seTh += e(2, 0) * e(2, 0);
            nees += (e.t() * inverse(P) * e)(0, 0);
            nis += (y.t() * inverse(S) * y)(0, 0);
            ++n;
        }
        if (k % 100 == 0 || k == 1) {
            std::cout << std::fixed << std::setprecision(1) << std::setw(5) << t
                      << std::setprecision(3) << std::setw(8) << s(0, 0) << std::setw(8) << s(1, 0)
                      << std::setw(8) << s(2, 0) << std::setw(8) << tx << std::setw(8) << ty
                      << std::setw(8) << tth << std::setw(7) << std::sqrt(P(0, 0)) << std::setw(7)
                      << std::sqrt(P(1, 1)) << std::setw(7) << std::sqrt(P(2, 2)) << '\n';
        }
    }
    std::cout << std::setprecision(4) << "\nsteps 21-600:\n"
              << "  RMS position error " << std::sqrt(sePos / n) << " m, RMS heading error "
              << std::sqrt(seTh / n) * 180.0 / kPi << " deg\n"
              << "  mean NEES (3 states; consistent: about 3): " << nees / n << '\n'
              << "  mean NIS  (2 readings; consistent: about 2): " << nis / n << '\n'
              << "odometry alone, final position error: "
              << std::hypot(ox - lastX, oy - lastY) << " m; EKF final position error: "
              << std::hypot(s(0, 0) - lastX, s(1, 0) - lastY) << " m\n";
    return 0;
}
