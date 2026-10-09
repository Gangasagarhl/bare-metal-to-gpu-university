// F0-60 forensic evidence: Jonas averages two thermometers to halve the noise variance.
// Both thermometers hang on the same wall and share one power adapter (see the answer key).
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

double gaussian(std::mt19937& engine)
{
    const double u1 = uniform01(engine);
    const double u2 = uniform01(engine);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * std::acos(-1.0) * u2);
}

double sd(const std::vector<double>& v)
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
    std::mt19937 engine(6060);
    std::vector<double> a;
    std::vector<double> b;
    std::vector<double> average;
    std::vector<double> difference;
    for (int i = 0; i < 5000; ++i) {
        const double shared = 0.27 * gaussian(engine);   // ripple from the shared adapter
        const double ta = 21.0 + shared + 0.10 * gaussian(engine);
        const double tb = 21.0 + shared + 0.10 * gaussian(engine);
        a.push_back(ta);
        b.push_back(tb);
        average.push_back((ta + tb) / 2.0);
        difference.push_back(ta - tb);
    }
    std::cout << "first readings (A, B):";
    for (int i = 0; i < 6; ++i) {
        std::cout << " (" << std::round(a[static_cast<std::size_t>(i)] * 100) / 100 << ", "
                  << std::round(b[static_cast<std::size_t>(i)] * 100) / 100 << ")";
    }
    std::cout << "\nreadings: " << a.size() << " pairs\n";
    std::cout << "sd of A:            " << sd(a) << " C\n";
    std::cout << "sd of B:            " << sd(b) << " C\n";
    std::cout << "sd of (A + B) / 2:  " << sd(average) << " C\n";
    std::cout << "sd of A - B:        " << sd(difference) << " C\n";
    std::cout << "Jonas's expectation sd(A) / sqrt(2): " << sd(a) / std::sqrt(2.0) << " C\n";
    return 0;
}
