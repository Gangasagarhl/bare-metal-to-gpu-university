// F9-32 Listing 3: what happens when Q or R is scaled away from the measured values.
// For each setting: RMS error against ground truth, mean NIS and the lag-1 autocorrelation
// of the innovations (the last two need no ground truth, so a robot can compute them).
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
    std::getline(in, line);
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

void run(const std::vector<Sample>& drive, double qScale, double rScale)
{
    const double Q = 0.000344 * qScale;  // tuned values printed by Listing 2
    const double R = 0.007044 * rScale;
    double x = 0.0, P = 1.0;
    double se = 0.0, nisSum = 0.0;
    std::vector<double> nu;
    for (std::size_t k = 0; k < drive.size(); ++k) {
        x += drive[k].odo * 0.1;
        P += Q;
        const double S = P + R;
        const double v = drive[k].z - x;
        const double K = P / S;
        x += K * v;
        P = (1.0 - K) * P;
        if (k >= 10) {
            se += (x - drive[k].truth) * (x - drive[k].truth);
            nisSum += v * v / S;
            nu.push_back(v);
        }
    }
    double m = 0.0;
    for (double v : nu) {
        m += v;
    }
    m /= static_cast<double>(nu.size());
    double c0 = 0.0, c1 = 0.0;
    for (std::size_t i = 0; i < nu.size(); ++i) {
        c0 += (nu[i] - m) * (nu[i] - m);
        if (i > 0) {
            c1 += (nu[i] - m) * (nu[i - 1] - m);
        }
    }
    const double n = static_cast<double>(nu.size());
    std::cout << std::fixed << std::setprecision(2) << std::setw(8) << qScale << std::setw(8)
              << rScale << std::setprecision(4) << std::setw(10) << std::sqrt(se / n)
              << std::setw(10) << std::sqrt(P) << std::setw(10) << nisSum / n << std::setw(9)
              << c1 / c0 << '\n';
}

int main()
{
    const std::vector<Sample> drive = readDrive("cart_log.csv");
    std::cout << " Q scale R scale  RMS err  claimed sd  mean NIS  lag-1 r\n";
    for (double r : {0.01, 0.1, 1.0, 10.0, 100.0}) {
        run(drive, 1.0, r);
    }
    std::cout << '\n';
    for (double q : {0.01, 0.1, 10.0, 100.0}) {
        run(drive, q, 1.0);
    }
    return 0;
}
