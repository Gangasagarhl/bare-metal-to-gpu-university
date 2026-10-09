// F0-57 Listing 2: a continuous random variable. Histogram of 10000 uniform values on
// [0, 1), the density estimate count / (N * width), and P(0.2 < U < 0.5) as an area.
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

// Uniform value in (0, 1) from 32 random bits: the centre of one of 2^32 equal slices.
double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

void printHistogram(const std::vector<double>& values, double lo, double hi, int bins)
{
    std::vector<int> count(static_cast<std::size_t>(bins), 0);
    const double width = (hi - lo) / bins;
    for (double v : values) {
        int b = static_cast<int>((v - lo) / width);
        if (b == bins) {
            b = bins - 1;  // a value exactly equal to hi belongs to the last bin
        }
        ++count[static_cast<std::size_t>(b)];
    }
    std::cout << "bin            count  density\n";
    for (int b = 0; b < bins; ++b) {
        const double left = lo + b * width;
        const int c = count[static_cast<std::size_t>(b)];
        const double density = c / (values.size() * width);
        std::cout << "[" << left << ", " << left + width << ")\t" << c << "\t" << density << "\n";
    }
}

int main()
{
    std::mt19937 engine(2057);
    const int n = 10000;
    std::vector<double> u;
    std::vector<double> sumOfTwo;
    for (int i = 0; i < n; ++i) {
        u.push_back(uniform01(engine));
        sumOfTwo.push_back(uniform01(engine) + uniform01(engine));
    }
    std::cout << "U uniform on [0, 1), N = " << n << " (seed 2057)\n";
    printHistogram(u, 0.0, 1.0, 10);
    int inside = 0;
    for (double v : u) {
        if (v > 0.2 && v < 0.5) {
            ++inside;
        }
    }
    std::cout << "frequency of 0.2 < U < 0.5: " << static_cast<double>(inside) / n
              << "   (exact area under the density: 0.5 - 0.2 = 0.3)\n\n";
    std::cout << "S = U1 + U2 on [0, 2), N = " << n << "\n";
    printHistogram(sumOfTwo, 0.0, 2.0, 10);
    return 0;
}
