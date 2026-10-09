// F0-60 Listing 2: correlation of four simulated pairs. Strong, negative, none, and
// "none although y is completely determined by x".
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

double correlation(const std::vector<double>& x, const std::vector<double>& y)
{
    const double n = static_cast<double>(x.size());
    double mx = 0.0;
    double my = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        mx += x[i] / n;
        my += y[i] / n;
    }
    double sxx = 0.0;
    double syy = 0.0;
    double sxy = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        sxx += (x[i] - mx) * (x[i] - mx);
        syy += (y[i] - my) * (y[i] - my);
        sxy += (x[i] - mx) * (y[i] - my);
    }
    return sxy / std::sqrt(sxx * syy);
}

int main()
{
    std::mt19937 engine(60);
    const int n = 10000;
    std::vector<double> x;
    std::vector<double> linear;
    std::vector<double> falling;
    std::vector<double> unrelated;
    std::vector<double> square;
    for (int i = 0; i < n; ++i) {
        const double xi = 2.0 * uniform01(engine) - 1.0;   // uniform on (-1, 1)
        x.push_back(xi);
        linear.push_back(2.0 * xi + 0.5 * (2.0 * uniform01(engine) - 1.0));
        falling.push_back(-xi + 1.0 * (2.0 * uniform01(engine) - 1.0));
        unrelated.push_back(2.0 * uniform01(engine) - 1.0);
        square.push_back(xi * xi);
    }
    std::cout << "r(x, 2x + small noise)  = " << correlation(x, linear) << "\n";
    std::cout << "r(x, -x + large noise)  = " << correlation(x, falling) << "\n";
    std::cout << "r(x, independent value) = " << correlation(x, unrelated) << "\n";
    std::cout << "r(x, x squared)         = " << correlation(x, square) << "\n";
    return 0;
}
