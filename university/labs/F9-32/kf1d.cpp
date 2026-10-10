// F9-32 Listing 2: the course lab. A one-dimensional Kalman filter on recorded data with
// ground truth. R is measured from the rest recording (the MA202 F0-62 method); Q is
// measured from the ground-truth part of the drive recording. Then the filter runs and is
// compared with the raw range readings and with odometry alone.
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Sample
{
    double t, odo, z, truth;
};

std::vector<Sample> readDrive(const std::string& path)
{
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);  // header
    std::vector<Sample> out;
    while (std::getline(in, line)) {
        std::istringstream fields(line);
        Sample s{};
        char comma = ',';
        fields >> s.t >> comma >> s.odo >> comma >> s.z >> comma >> s.truth;
        out.push_back(s);
    }
    return out;
}

double sampleVariance(const std::vector<double>& v)
{
    double mean = 0.0;
    for (double x : v) {
        mean += x;
    }
    mean /= static_cast<double>(v.size());
    double ss = 0.0;
    for (double x : v) {
        ss += (x - mean) * (x - mean);
    }
    return ss / static_cast<double>(v.size() - 1);
}

int main()
{
    const double dt = 0.1;
    // 1. Tune R: range readings at a known position (sensor noise only).
    std::ifstream rest("rest_log.csv");
    std::string line;
    std::getline(rest, line);
    std::vector<double> ranges;
    while (std::getline(rest, line)) {
        ranges.push_back(std::stod(line));
    }
    const double R = sampleVariance(ranges);

    // 2. Tune Q: how far each true step differs from what odometry predicted.
    const std::vector<Sample> drive = readDrive("cart_log.csv");
    std::vector<double> stepErrors;
    double previousTruth = 0.5;  // start position, tape-measured
    for (const Sample& s : drive) {
        stepErrors.push_back((s.truth - previousTruth) - s.odo * dt);
        previousTruth = s.truth;
    }
    const double Q = sampleVariance(stepErrors);
    std::cout << std::fixed << std::setprecision(6) << "tuned R = " << R << " m^2 (sd "
              << std::sqrt(R) << " m, from " << ranges.size() << " rest readings)\n"
              << "tuned Q = " << Q << " m^2 (sd " << std::sqrt(Q) << " m per step)\n\n";

    // 3. The filter.
    double x = 0.0;   // initial guess
    double P = 1.0;   // initial variance: "somewhere within a couple of metres"
    double odoOnly = 0.5;  // odometry alone, started at the tape-measured position
    double seFilter = 0.0, seRaw = 0.0, seOdo = 0.0, nisSum = 0.0;
    int inside2sd = 0;
    std::cout << "    t      z   x_pred   P_pred      K    x_new    P_new   truth\n";
    for (std::size_t k = 0; k < drive.size(); ++k) {
        const Sample& s = drive[k];
        // Predict: move by the odometry, add process noise.
        x = x + s.odo * dt;
        P = P + Q;
        const double xPred = x;
        const double pPred = P;
        odoOnly += s.odo * dt;
        // Update: blend in the range reading.
        const double innovation = s.z - x;
        const double S = P + R;
        const double K = P / S;
        x = x + K * innovation;
        P = (1.0 - K) * P;

        const double err = x - s.truth;
        if (k >= 10) {  // skip the start-up transient in the statistics
            seFilter += err * err;
            seRaw += (s.z - s.truth) * (s.z - s.truth);
            seOdo += (odoOnly - s.truth) * (odoOnly - s.truth);
            nisSum += innovation * innovation / S;
            if (std::fabs(err) <= 2.0 * std::sqrt(P)) {
                ++inside2sd;
            }
        }
        if (k < 6 || k % 40 == 39) {
            std::cout << std::setprecision(3) << std::setw(5) << s.t << std::setw(8) << s.z
                      << std::setw(9) << xPred << std::setw(9) << std::setprecision(5) << pPred
                      << std::setprecision(3) << std::setw(7) << K << std::setw(9) << x
                      << std::setprecision(5) << std::setw(9) << P << std::setprecision(3)
                      << std::setw(8) << s.truth << '\n';
        }
    }
    const double n = static_cast<double>(drive.size() - 10);
    std::cout << "\nRMS error over steps 11-200 (m): range alone " << std::setprecision(4)
              << std::sqrt(seRaw / n) << ", odometry alone "
              << std::sqrt(seOdo / n) << ", Kalman filter " << std::sqrt(seFilter / n) << '\n'
              << "filter sd claimed at the end: " << std::sqrt(P) << " m\n"
              << "errors inside the filter's own 2-sd band: " << inside2sd << " of "
              << static_cast<int>(n) << '\n'
              << "mean normalised innovation squared (should be near 1): "
              << nisSum / n << '\n';
    return 0;
}
