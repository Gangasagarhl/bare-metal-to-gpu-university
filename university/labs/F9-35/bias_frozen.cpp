// F9-35 forensic evidence generator: "The robot that slowly turns away".
// The fusion EKF of Listing 2 (with the gate) as shipped in release 2.0: the gyro bias was
// "calibrated on the bench" and the configuration gives the bias state an initial sd of 0
// and no random walk. The program prints the robot's own diagnostics in 20 s windows.
#include "mat.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const double kPi = std::acos(-1.0);

int main()
{
    const double dt = 0.1;
    const double gyroSd = 0.00174, odoWSd = 0.02, vSd = 0.01;  // as tuned in Listing 2
    Mat s(5, 1);
    Mat P(5, 5);
    P(3, 3) = 0.01;
    P(4, 4) = 0.0;                 // release 2.0: "bias is known"
    const double biasStep = 0.0;   // release 2.0: "bias never changes"
    std::ifstream in("drive_log.csv");
    std::string line;
    std::getline(in, line);
    std::cout << "window   mean odo innovation  sd odo innovation  mean odo NIS  rejected"
                 "  bias est   sd(bias)\n"
              << "(s)      (rad/s)              (rad/s)                                    "
                 "  (deg/s)    (deg/s)\n";
    double sumY = 0.0, sumYY = 0.0, sumNis = 0.0, tx = 0, ty = 0, tth = 0;
    int count = 0, rejected = 0, k = 0;
    while (std::getline(in, line)) {
        std::istringstream f(line);
        double t, v, wOdo, gyro;
        int slip;
        char c;
        f >> t >> c >> v >> c >> wOdo >> c >> gyro >> c >> tx >> c >> ty >> c >> tth >> c >> slip;
        ++k;
        const double th = s(2, 0);
        const Mat G(5, 5, {1, 0, -v * dt * std::sin(th), 0, 0,
                           0, 1, v * dt * std::cos(th), 0, 0,
                           0, 0, 1, dt, 0,
                           0, 0, 0, 1, 0,
                           0, 0, 0, 0, 1});
        Mat Q(5, 5);
        Q(0, 0) = std::pow(vSd * dt * std::cos(th), 2);
        Q(0, 1) = Q(1, 0) = vSd * vSd * dt * dt * std::cos(th) * std::sin(th);
        Q(1, 1) = std::pow(vSd * dt * std::sin(th), 2);
        Q(3, 3) = 0.05 * 0.05;
        Q(4, 4) = biasStep * biasStep;
        s = Mat(5, 1, {s(0, 0) + v * dt * std::cos(th), s(1, 0) + v * dt * std::sin(th),
                       th + s(3, 0) * dt, s(3, 0), s(4, 0)});
        P = G * P * G.t() + Q;
        auto update = [&](const Mat& H, double z, double var, double gate, double* yOut,
                          double* nisOut) {
            const double y = z - (H * s)(0, 0);
            const double S = (H * P * H.t())(0, 0) + var;
            *yOut = y;
            *nisOut = y * y / S;
            if (gate > 0.0 && y * y / S > gate) {
                return false;
            }
            const Mat K = (1.0 / S) * (P * H.t());
            s = s + y * K;
            const Mat IKH = Mat::identity(5) - K * H;
            P = IKH * P * IKH.t() + var * (K * K.t());
            return true;
        };
        double y = 0.0, nis = 0.0;
        update(Mat(1, 5, {0, 0, 0, 1, 1}), gyro, gyroSd * gyroSd, 0.0, &y, &nis);
        if (update(Mat(1, 5, {0, 0, 0, 1, 0}), wOdo, odoWSd * odoWSd, 9.0, &y, &nis)) {
            sumY += y;
            sumYY += y * y;
            sumNis += nis;
            ++count;
        } else {
            ++rejected;
        }
        if (k % 200 == 0) {
            const double mean = sumY / count;
            std::cout << std::fixed << std::setw(3) << static_cast<int>(t) - 20 << "-"
                      << std::setw(3)
                      << std::left << static_cast<int>(t) << std::right << std::setprecision(5)
                      << std::setw(14) << mean << std::setw(19)
                      << std::sqrt(sumYY / count - mean * mean) << std::setprecision(3)
                      << std::setw(16) << sumNis / count << std::setw(10) << rejected
                      << std::setw(11) << s(4, 0) * 180.0 / kPi << std::setw(11)
                      << std::sqrt(P(4, 4)) * 180.0 / kPi << '\n';
            sumY = sumYY = sumNis = 0.0;
            count = rejected = 0;
        }
    }
    std::cout << "\nend of run (robot parked against the dock wall, heading checked with a "
                 "square):\n  estimated heading " << std::setprecision(1) << s(2, 0) * 180.0 / kPi
              << " deg, heading measured at the dock " << tth * 180.0 / kPi << " deg\n";
    return 0;
}
