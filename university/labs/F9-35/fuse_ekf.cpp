// F9-35 Listing 2: fusing wheel odometry and an IMU gyro with an EKF.
// State s = [x, y, theta, omega, b]: pose, true turn rate, gyro bias.
// Prediction: unicycle motion with the odometry speed v. Updates (both linear):
//   gyro      z_g = omega + b + noise      H_g = [0 0 0 1 1]
//   odometry  z_o = omega + noise          H_o = [0 0 0 1 0], skipped when it fails a gate
// The gyro noise is MEASURED from the MA202 rest recording (../F0-62/imu_rest.csv).
#include "mat.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

const double kPi = std::acos(-1.0);

struct Row
{
    double t, v, wOdo, gyro, tx, ty, tth;
    int slip;
};

double measuredGyroSd()  // sample sd of the gz column of the MA202 recording, in rad/s
{
    std::ifstream in("../F0-62/imu_rest.csv");
    std::string line;
    std::getline(in, line);
    std::vector<double> gz;
    while (std::getline(in, line)) {
        std::istringstream f(line);
        std::string cell;
        for (int i = 0; i < 7 && std::getline(f, cell, ','); ++i) {
            if (i == 6) {
                gz.push_back(std::stod(cell) * kPi / 180.0);
            }
        }
    }
    double m = 0.0;
    for (double g : gz) {
        m += g;
    }
    m /= static_cast<double>(gz.size());
    double ss = 0.0;
    for (double g : gz) {
        ss += (g - m) * (g - m);
    }
    return std::sqrt(ss / static_cast<double>(gz.size() - 1));
}

struct Result
{
    double rmsHeadingDeg, finalHeadingDeg, finalPosM, biasDegS;
    int rejected, rejectedInSlip;
};

// useGyro / useOdo choose the sensors; gate is the NIS threshold for odometry (0 = no gate).
Result run(const std::vector<Row>& rows, double gyroSd, bool useGyro, bool useOdo, double gate)
{
    const double dt = 0.1;
    Mat s(5, 1);  // starts at the origin, facing +x, everything else 0
    Mat P(5, 5);
    P(3, 3) = 0.01;
    P(4, 4) = std::pow(1.0 * kPi / 180.0, 2);  // bias unknown: sd 1 deg/s
    const double omegaStep = 0.05;     // rad/s change of the turn rate per step (model)
    const double biasStep = 1e-5;      // rad/s bias random walk per step (model)
    const double vSd = 0.01, odoWSd = 0.02;
    double se = 0.0;
    int rejected = 0, rejectedInSlip = 0;
    for (const Row& r : rows) {
        const double th = s(2, 0);
        const Mat G(5, 5, {1, 0, -r.v * dt * std::sin(th), 0, 0,
                           0, 1, r.v * dt * std::cos(th), 0, 0,
                           0, 0, 1, dt, 0,
                           0, 0, 0, 1, 0,
                           0, 0, 0, 0, 1});
        Mat Q(5, 5);
        Q(0, 0) = std::pow(vSd * dt * std::cos(th), 2);
        Q(0, 1) = Q(1, 0) = vSd * vSd * dt * dt * std::cos(th) * std::sin(th);
        Q(1, 1) = std::pow(vSd * dt * std::sin(th), 2);
        Q(3, 3) = omegaStep * omegaStep;
        Q(4, 4) = biasStep * biasStep;
        s = Mat(5, 1, {s(0, 0) + r.v * dt * std::cos(th), s(1, 0) + r.v * dt * std::sin(th),
                       th + s(3, 0) * dt, s(3, 0), s(4, 0)});
        P = G * P * G.t() + Q;

        auto update = [&](const Mat& H, double z, double var, double gateLimit) {
            const double y = z - (H * s)(0, 0);
            const double S = (H * P * H.t())(0, 0) + var;
            if (gateLimit > 0.0 && y * y / S > gateLimit) {
                return false;  // reading disagrees too much with the prediction: reject it
            }
            const Mat K = (1.0 / S) * (P * H.t());
            s = s + y * K;
            const Mat IKH = Mat::identity(5) - K * H;
            P = IKH * P * IKH.t() + var * (K * K.t());
            return true;
        };
        if (useGyro) {
            update(Mat(1, 5, {0, 0, 0, 1, 1}), r.gyro, gyroSd * gyroSd, 0.0);
        }
        if (useOdo && !update(Mat(1, 5, {0, 0, 0, 1, 0}), r.wOdo, odoWSd * odoWSd, gate)) {
            ++rejected;
            rejectedInSlip += r.slip;
        }
        const double e = s(2, 0) - r.tth;
        se += e * e;
    }
    const Row& last = rows.back();
    return {std::sqrt(se / static_cast<double>(rows.size())) * 180.0 / kPi,
            (s(2, 0) - last.tth) * 180.0 / kPi, std::hypot(s(0, 0) - last.tx, s(1, 0) - last.ty),
            s(4, 0) * 180.0 / kPi, rejected, rejectedInSlip};
}

int main()
{
    const double gyroSd = measuredGyroSd();
    std::cout << std::fixed << std::setprecision(5) << "gyro noise sd from ../F0-62/imu_rest.csv: "
              << gyroSd << " rad/s (" << gyroSd * 180.0 / kPi << " deg/s)\n\n";
    std::ifstream in("drive_log.csv");
    std::string line;
    std::getline(in, line);
    std::vector<Row> rows;
    while (std::getline(in, line)) {
        std::istringstream f(line);
        Row r{};
        char c;
        f >> r.t >> c >> r.v >> c >> r.wOdo >> c >> r.gyro >> c >> r.tx >> c >> r.ty >> c >> r.tth
            >> c >> r.slip;
        rows.push_back(r);
    }
    int slipSteps = 0;
    for (const Row& r : rows) {
        slipSteps += r.slip;
    }
    std::cout << "samples recorded on a slippery patch: " << slipSteps << "\n\n";
    struct Case
    {
        const char* name;
        bool gyro, odo;
        double gate;
    };
    const Case cases[] = {{"odometry only", false, true, 0.0},
                          {"gyro only (bias unknown)", true, false, 0.0},
                          {"fusion, no gate", true, true, 0.0},
                          {"fusion, gate NIS > 9", true, true, 9.0}};
    std::cout << "estimator                                 RMS heading  final heading  final pos"
                 "  bias est.  rejected odometry\n"
              << "                                          error (deg)  error (deg)    error (m)"
                 "  (deg/s)    updates (in slip)\n";
    for (const Case& c : cases) {
        const Result res = run(rows, gyroSd, c.gyro, c.odo, c.gate);
        std::cout << std::left << std::setw(42) << c.name << std::right << std::setprecision(3)
                  << std::setw(11) << res.rmsHeadingDeg << std::setw(14) << res.finalHeadingDeg
                  << std::setw(12) << res.finalPosM << std::setw(11) << res.biasDegS
                  << std::setw(7) << res.rejected << " (" << res.rejectedInSlip << ")\n";
    }
    return 0;
}
