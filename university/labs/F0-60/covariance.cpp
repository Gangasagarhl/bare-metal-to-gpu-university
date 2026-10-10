// F0-60 Listing 1: covariance and correlation of paired data, step by step.
// Input: pairs "x y" per line (covariance.in: hours of sunshine, kitchen temperature at 3 pm).
#include <cmath>
#include <iostream>
#include <vector>

struct Pair
{
    double x;
    double y;
};

int main()
{
    std::vector<Pair> data;
    Pair p{};
    while (std::cin >> p.x >> p.y) {
        data.push_back(p);
    }
    const double n = static_cast<double>(data.size());
    double sumX = 0.0;
    double sumY = 0.0;
    for (const Pair& d : data) {
        sumX += d.x;
        sumY += d.y;
    }
    const double meanX = sumX / n;
    const double meanY = sumY / n;

    double sxx = 0.0;
    double syy = 0.0;
    double sxy = 0.0;
    std::cout << "x   y    dx     dy     dx*dy\n";
    for (const Pair& d : data) {
        const double dx = d.x - meanX;
        const double dy = d.y - meanY;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
        std::cout << d.x << "   " << d.y << "   " << dx << "\t" << dy << "\t" << dx * dy << "\n";
    }
    const double covXY = sxy / (n - 1.0);
    const double sdX = std::sqrt(sxx / (n - 1.0));
    const double sdY = std::sqrt(syy / (n - 1.0));
    std::cout << "mean x = " << meanX << ", mean y = " << meanY << "\n";
    std::cout << "sample covariance = " << sxy << " / " << n - 1.0 << " = " << covXY << "\n";
    std::cout << "sd x = " << sdX << ", sd y = " << sdY << "\n";
    std::cout << "correlation r = cov / (sd x * sd y) = " << covXY / (sdX * sdY) << "\n";
    std::cout << "covariance matrix [[var x, cov], [cov, var y]] = [[" << sdX * sdX << ", "
              << covXY << "], [" << covXY << ", " << sdY * sdY << "]]\n";
    return 0;
}
