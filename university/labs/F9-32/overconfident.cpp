// F9-32 forensic evidence generator: "The overconfident filter".
// Runs the cart filter of Listing 2 twice on the same recording: release 1.3 (healthy) and
// release 1.4 (after a configuration change the developer did not mention). It prints only
// what the robot itself can know: innovations, gains and claimed uncertainty. No ground truth.
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

void report(const std::string& release, const std::vector<Sample>& drive, double Q, double R)
{
    const double dt = 0.1;
    double x = 0.0;
    double P = 1.0;
    std::vector<double> nu;     // innovations
    std::vector<double> nis;    // normalised innovations squared
    std::vector<double> parked; // estimates while the operator saw the cart stopped (5-8 s)
    double K = 0.0;
    for (const Sample& s : drive) {
        x += s.odo * dt;
        P += Q;
        const double S = P + R;
        const double v = s.z - x;
        K = P / S;
        x += K * v;
        P = (1.0 - K) * P;
        if (s.t > 1.05) {  // skip the first second (start-up)
            nu.push_back(v);
            nis.push_back(v * v / S);
        }
        if (s.t > 5.05 && s.t < 7.95) {
            parked.push_back(x);
        }
    }
    double meanNis = 0.0, meanNu = 0.0;
    int outside = 0;
    for (std::size_t i = 0; i < nu.size(); ++i) {
        meanNis += nis[i];
        meanNu += nu[i];
        if (nis[i] > 4.0) {
            ++outside;  // |innovation| > 2 sqrt(S)
        }
    }
    meanNis /= static_cast<double>(nu.size());
    meanNu /= static_cast<double>(nu.size());
    double c0 = 0.0, c1 = 0.0;
    for (std::size_t i = 0; i < nu.size(); ++i) {
        c0 += (nu[i] - meanNu) * (nu[i] - meanNu);
        if (i > 0) {
            c1 += (nu[i] - meanNu) * (nu[i - 1] - meanNu);
        }
    }
    double pm = 0.0;
    for (double p : parked) {
        pm += p;
    }
    pm /= static_cast<double>(parked.size());
    double ps = 0.0;
    for (double p : parked) {
        ps += (p - pm) * (p - pm);
    }
    std::cout << std::fixed << std::setprecision(4) << "release " << release << '\n'
              << "  steady-state gain K                      " << K << '\n'
              << "  claimed position sd sqrt(P)  (m)         " << std::sqrt(P) << '\n'
              << "  predicted innovation sd sqrt(S)  (m)     "
              << std::sqrt(P / (1.0 - K) + R) << '\n'
              << "  measured innovation sd  (m)              "
              << std::sqrt(c0 / static_cast<double>(nu.size() - 1)) << '\n'
              << "  mean innovation  (m)                     " << meanNu << '\n'
              << "  mean NIS (innovation^2 / S)              " << meanNis << '\n'
              << "  innovations outside +-2 sqrt(S)          " << outside << " of " << nu.size()
              << '\n'
              << "  lag-1 autocorrelation of innovations     " << c1 / c0 << '\n'
              << "  sd of the estimate while parked (m)      "
              << std::sqrt(ps / static_cast<double>(parked.size() - 1)) << " over "
              << parked.size() << " samples\n";
}

int main()
{
    const std::vector<Sample> drive = readDrive("cart_log.csv");
    report("1.3", drive, 0.000344, 0.007044);
    report("1.4", drive, 0.000344, 0.000064);
    return 0;
}
