// F0-59 Listing 1: Gaussian samples from uniform ones (Box-Muller), checked against the
// density and against the exact fractions within 1, 2 and 3 standard deviations.
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

const double pi = std::acos(-1.0);

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

// Box-Muller: two independent uniforms give one standard Gaussian value (we use one of two).
double standardGaussian(std::mt19937& engine)
{
    const double u1 = uniform01(engine);
    const double u2 = uniform01(engine);
    return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * pi * u2);
}

double gaussianPdf(double x, double mu, double sigma)
{
    const double z = (x - mu) / sigma;
    return std::exp(-0.5 * z * z) / (sigma * std::sqrt(2.0 * pi));
}

int main()
{
    std::mt19937 engine(59);
    const double mu = 20.0;    // kitchen temperature model, degrees C (exercise values)
    const double sigma = 0.5;
    const int n = 100000;
    std::vector<double> x;
    for (int i = 0; i < n; ++i) {
        x.push_back(mu + sigma * standardGaussian(engine));
    }

    std::cout << "N(" << mu << ", " << sigma << "^2), " << n << " samples, seed 59\n";
    std::cout << "bin centre  density estimate  pdf\n";
    const double width = 0.25;
    for (double c = 18.75; c < 21.3; c += width) {
        int count = 0;
        for (double v : x) {
            if (v >= c - width / 2 && v < c + width / 2) {
                ++count;
            }
        }
        std::cout << c << "\t    " << count / (n * width) << "\t\t  " << gaussianPdf(c, mu, sigma)
                  << "\n";
    }

    std::cout << "k   fraction within k sigma   exact erf(k / sqrt 2)\n";
    for (int k = 1; k <= 3; ++k) {
        int inside = 0;
        for (double v : x) {
            if (std::fabs(v - mu) < k * sigma) {
                ++inside;
            }
        }
        std::cout << k << "   " << static_cast<double>(inside) / n << "\t\t\t     "
                  << std::erf(k / std::sqrt(2.0)) << "\n";
    }
    return 0;
}
