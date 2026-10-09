// F0-62 Listing 2: noise statistics of an IMU lying still. Reads imu_rest.csv (written by
// Listing 1, or recorded from a real IMU in the same format) and prints, per axis:
// mean, bias (mean minus the value expected at rest), standard deviation, the standard
// error of the mean, min, max and the number of distinct values; then r(ax, ay).
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

int main()
{
    std::ifstream in("imu_rest.csv");
    std::string line;
    std::getline(in, line);  // header
    std::vector<std::vector<double>> col(6);
    while (std::getline(in, line)) {
        std::stringstream fields(line);
        std::string cell;
        std::getline(fields, cell, ',');  // time column, not needed here
        for (int axis = 0; axis < 6; ++axis) {
            std::getline(fields, cell, ',');
            col[static_cast<std::size_t>(axis)].push_back(std::stod(cell));
        }
    }
    const char* names[6] = {"ax", "ay", "az", "gx", "gy", "gz"};
    const double expected[6] = {0.0, 0.0, 9.81, 0.0, 0.0, 0.0};  // at rest and level
    std::vector<double> mean(6);
    std::cout << "samples: " << col[0].size() << "\n";
    std::cout << "axis  mean      bias      sd        sd/sqrt(n)  min      max      distinct\n";
    for (std::size_t a = 0; a < 6; ++a) {
        const std::vector<double>& x = col[a];
        const double n = static_cast<double>(x.size());
        double sum = 0.0;
        for (double v : x) {
            sum += v;
        }
        mean[a] = sum / n;
        double ss = 0.0;
        double lo = x[0];
        double hi = x[0];
        for (double v : x) {
            ss += (v - mean[a]) * (v - mean[a]);
            lo = std::fmin(lo, v);
            hi = std::fmax(hi, v);
        }
        const double sd = std::sqrt(ss / (n - 1.0));
        const std::set<double> distinct(x.begin(), x.end());
        std::cout << names[a] << "    " << mean[a] << "  " << mean[a] - expected[a] << "  " << sd
                  << "  " << sd / std::sqrt(n) << "  " << lo << "  " << hi << "  "
                  << distinct.size() << "\n";
    }
    double sxy = 0.0;
    double sxx = 0.0;
    double syy = 0.0;
    for (std::size_t i = 0; i < col[0].size(); ++i) {
        const double dx = col[0][i] - mean[0];
        const double dy = col[1][i] - mean[1];
        sxy += dx * dy;
        sxx += dx * dx;
        syy += dy * dy;
    }
    std::cout << "correlation r(ax, ay) = " << sxy / std::sqrt(sxx * syy) << "\n";
    double sxz = 0.0;
    double szz = 0.0;
    for (std::size_t i = 0; i < col[0].size(); ++i) {
        sxz += (col[0][i] - mean[0]) * (col[2][i] - mean[2]);
        szz += (col[2][i] - mean[2]) * (col[2][i] - mean[2]);
    }
    std::cout << "correlation r(ax, az) = " << sxz / std::sqrt(sxx * szz) << "\n";
    return 0;
}
