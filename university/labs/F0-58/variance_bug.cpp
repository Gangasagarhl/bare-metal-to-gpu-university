// F0-58 forensic evidence: three ways to compute the variance of the same readings.
// Readings from a slowly counting sensor: 100000 plus a small wobble of -2..+2.
#include <cmath>
#include <iostream>
#include <vector>

int main()
{
    std::vector<double> readings;
    for (int i = 0; i < 1000; ++i) {
        readings.push_back(100000.0 + static_cast<double>(i % 5) - 2.0);
    }
    const std::size_t n = readings.size();

    // A: Zoe's one-pass "mean of squares minus square of mean", in float.
    float sumA = 0.0f;
    float sumSqA = 0.0f;
    for (double r : readings) {
        const float x = static_cast<float>(r);
        sumA += x;
        sumSqA += x * x;
    }
    const float meanA = sumA / static_cast<float>(n);
    const float varA = sumSqA / static_cast<float>(n) - meanA * meanA;

    // B: the same formula in double.
    double sumB = 0.0;
    double sumSqB = 0.0;
    for (double r : readings) {
        sumB += r;
        sumSqB += r * r;
    }
    const double meanB = sumB / static_cast<double>(n);
    const double varB = sumSqB / static_cast<double>(n) - meanB * meanB;

    // C: two passes in double: first the mean, then the squared deviations.
    double ss = 0.0;
    for (double r : readings) {
        ss += (r - meanB) * (r - meanB);
    }
    const double varC = ss / static_cast<double>(n);

    std::cout << "n = " << n << " readings, values 99998 .. 100002\n";
    std::cout << "A  float,  mean of squares - square of mean: mean " << meanA
              << ", variance " << varA << "\n";
    std::cout << "B  double, mean of squares - square of mean: mean " << meanB
              << ", variance " << varB << "\n";
    std::cout << "C  double, two passes:                       mean " << meanB
              << ", variance " << varC << "\n";
    // D: the double one-pass formula again, on the same wobble around 1000000000.
    double sumD = 0.0;
    double sumSqD = 0.0;
    for (double r : readings) {
        const double x = r - 100000.0 + 1000000000.0;
        sumD += x;
        sumSqD += x * x;
    }
    const double meanD = sumD / static_cast<double>(n);
    std::cout << "D  double, same formula, values around 1000000000: variance "
              << sumSqD / static_cast<double>(n) - meanD * meanD << "\n";
    std::cout << "std dev from A: " << (varA >= 0.0f ? std::sqrt(varA) : -1.0f)
              << "  (-1 means the variance was negative)\n";
    return 0;
}
