// F0-59 Listing 2: why Gaussians appear (sums of many small independent parts), and a
// skewed variable that is NOT Gaussian although it has a mean and a standard deviation.
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

void report(const char* name, const std::vector<double>& x)
{
    double sum = 0.0;
    for (double v : x) {
        sum += v;
    }
    const double mean = sum / static_cast<double>(x.size());
    double ss = 0.0;
    for (double v : x) {
        ss += (v - mean) * (v - mean);
    }
    const double sd = std::sqrt(ss / static_cast<double>(x.size() - 1));
    int within1 = 0;
    int within2 = 0;
    int above3 = 0;
    for (double v : x) {
        const double z = (v - mean) / sd;
        within1 += std::fabs(z) < 1.0 ? 1 : 0;
        within2 += std::fabs(z) < 2.0 ? 1 : 0;
        above3 += z > 3.0 ? 1 : 0;
    }
    const double n = static_cast<double>(x.size());
    std::cout << name << ": mean " << mean << ", sd " << sd << ", within 1 sd " << within1 / n
              << ", within 2 sd " << within2 / n << ", above +3 sd " << above3 / n << "\n";
}

int main()
{
    std::mt19937 engine(2059);
    const int n = 200000;
    std::vector<double> one;
    std::vector<double> twelve;
    std::vector<double> skewed;
    for (int i = 0; i < n; ++i) {
        one.push_back(uniform01(engine));
        double s = 0.0;
        for (int k = 0; k < 12; ++k) {
            s += uniform01(engine);
        }
        twelve.push_back(s - 6.0);                        // mean 0, variance 12 * (1/12) = 1
        skewed.push_back(-std::log(uniform01(engine)));   // exponential, mean 1, sd 1
    }
    report("one uniform        ", one);
    report("sum of 12 uniforms ", twelve);
    report("exponential (skew) ", skewed);
    std::cout << "Gaussian reference : within 1 sd " << std::erf(1.0 / std::sqrt(2.0))
              << ", within 2 sd " << std::erf(2.0 / std::sqrt(2.0)) << ", above +3 sd "
              << 0.5 * std::erfc(3.0 / std::sqrt(2.0)) << "\n";
    return 0;
}
