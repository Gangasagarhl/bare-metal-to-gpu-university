// F0-58 Listing 1: mean, deviations, variance and standard deviation of Ravi's kettle times.
// Reads one number per line from standard input (kettle.in: ten times in seconds).
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    std::vector<double> x;
    double value = 0.0;
    while (std::cin >> value) {
        x.push_back(value);
    }
    const double n = static_cast<double>(x.size());

    double sum = 0.0;
    for (double v : x) {
        sum += v;
    }
    const double mean = sum / n;

    double sumSquaredDeviations = 0.0;  // second pass: deviations from the known mean
    std::cout << "value  deviation  squared\n";
    for (double v : x) {
        const double d = v - mean;
        sumSquaredDeviations += d * d;
        std::cout << v << "    " << d << "\t" << d * d << "\n";
    }
    const double populationVariance = sumSquaredDeviations / n;
    const double sampleVariance = sumSquaredDeviations / (n - 1.0);

    std::cout << "n = " << n << ", sum = " << sum << ", mean = " << mean << " s\n";
    std::cout << "sum of squared deviations = " << sumSquaredDeviations << " s^2\n";
    std::cout << "variance, divide by n     = " << populationVariance << " s^2, std dev = "
              << std::sqrt(populationVariance) << " s\n";
    std::cout << "variance, divide by n - 1 = " << sampleVariance << " s^2, std dev = "
              << std::sqrt(sampleVariance) << " s\n";
    return 0;
}
