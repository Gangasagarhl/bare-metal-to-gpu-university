// F0-62 Listing 3: what averaging removes and what it cannot remove.
// A: block averages of the gyro x column of imu_rest.csv (noise shrinks, bias stays).
// B: quantisation noise of a step of 0.06 has sd step / sqrt(12).
// C: a constant input smaller than half a step, without noise, is never seen at all.
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

double sampleSd(const std::vector<double>& v)
{
    double m = 0.0;
    for (double x : v) {
        m += x / static_cast<double>(v.size());
    }
    double ss = 0.0;
    for (double x : v) {
        ss += (x - m) * (x - m);
    }
    return std::sqrt(ss / static_cast<double>(v.size() - 1));
}

int main()
{
    std::ifstream in("imu_rest.csv");
    std::string line;
    std::getline(in, line);
    std::vector<double> gx;
    while (std::getline(in, line)) {
        std::stringstream fields(line);
        std::string cell;
        for (int c = 0; c < 5; ++c) {
            std::getline(fields, cell, ',');  // t, ax, ay, az, gx: keep the last one
        }
        gx.push_back(std::stod(cell));
    }
    const double sd1 = sampleSd(gx);
    std::cout << "A  gyro x, " << gx.size() << " samples\n";
    std::cout << "   k    blocks  mean of block means  sd of block means  sd1/sqrt(k)\n";
    for (std::size_t k : {1u, 4u, 16u, 64u}) {
        std::vector<double> means;
        for (std::size_t start = 0; start + k <= gx.size(); start += k) {
            double s = 0.0;
            for (std::size_t i = start; i < start + k; ++i) {
                s += gx[i];
            }
            means.push_back(s / static_cast<double>(k));
        }
        double mm = 0.0;
        for (double m : means) {
            mm += m / static_cast<double>(means.size());
        }
        std::cout << "   " << k << "\t" << means.size() << "\t" << mm << "\t\t\t" << sampleSd(means)
                  << "\t\t   " << sd1 / std::sqrt(static_cast<double>(k)) << "\n";
    }

    const double step = 0.06;
    std::vector<double> error;
    for (int i = 0; i < 100000; ++i) {
        const double value = -1.0 + 2.0 * (i + 0.5) / 100000.0;   // values spread evenly
        error.push_back(step * std::round(value / step) - value);
    }
    std::cout << "B  quantisation error, step " << step << ": sd " << sampleSd(error)
              << ", step / sqrt(12) = " << step / std::sqrt(12.0) << "\n";

    const double tiny = 0.02;   // a true constant rate smaller than step / 2
    double sum = 0.0;
    for (int i = 0; i < 1000; ++i) {
        sum += step * std::round(tiny / step);
    }
    std::cout << "C  true value " << tiny << ", no noise, mean of 1000 quantised readings: "
              << sum / 1000.0 << "\n";
    return 0;
}
