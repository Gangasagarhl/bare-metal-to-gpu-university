// F9-36 Listing 1: importance weights, effective sample size and systematic resampling,
// on six particles small enough to follow by hand.
#include <iomanip>
#include <iostream>
#include <vector>

// Systematic (low-variance) resampling: one random offset u0 in [0, 1/N), then N equally
// spaced pointers walk along the cumulative weights. Returns the chosen particle indices.
std::vector<int> systematicResample(const std::vector<double>& w, double u0)
{
    const int n = static_cast<int>(w.size());
    std::vector<int> chosen;
    double cumulative = w[0];
    int i = 0;
    for (int m = 0; m < n; ++m) {
        const double pointer = u0 + static_cast<double>(m) / n;
        while (pointer > cumulative && i < n - 1) {
            ++i;
            cumulative += w[static_cast<std::size_t>(i)];
        }
        chosen.push_back(i);
    }
    return chosen;
}

int main()
{
    const std::vector<double> position = {1.3, 4.1, 6.6, 11.2, 16.4, 25.0};  // metres
    const std::vector<double> likelihood = {0.9, 0.9, 0.05, 0.9, 0.05, 0.05};  // p(z = door | x)
    std::vector<double> w(position.size());
    double total = 0.0;
    for (std::size_t i = 0; i < w.size(); ++i) {
        w[i] = likelihood[i] * (1.0 / 6.0);  // previous weights were equal
        total += w[i];
    }
    double sumSq = 0.0;
    std::cout << std::fixed << std::setprecision(4) << "particle  position  weight\n";
    for (std::size_t i = 0; i < w.size(); ++i) {
        w[i] /= total;
        sumSq += w[i] * w[i];
        std::cout << std::setw(8) << i << std::setw(10) << position[i] << std::setw(8) << w[i]
                  << '\n';
    }
    std::cout << "effective sample size 1 / sum(w^2) = " << 1.0 / sumSq << " of " << w.size()
              << '\n';
    const double u0 = 0.1;  // fixed here so the example can be checked by hand; random in a PF
    const std::vector<int> chosen = systematicResample(w, u0 / 6.0);
    std::cout << "pointers: ";
    for (int m = 0; m < 6; ++m) {
        std::cout << u0 / 6.0 + m / 6.0 << ' ';
    }
    std::cout << "\nresampled particles (indices): ";
    for (int c : chosen) {
        std::cout << c << ' ';
    }
    std::cout << "\nnew set of positions:          ";
    for (int c : chosen) {
        std::cout << position[static_cast<std::size_t>(c)] << ' ';
    }
    std::cout << "\n(all new weights are 1/6)\n";
    return 0;
}
