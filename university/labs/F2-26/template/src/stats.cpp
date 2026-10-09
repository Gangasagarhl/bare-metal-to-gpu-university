#include "uni/stats.h"

#include <algorithm>
#include <numeric>

namespace uni {

std::optional<double> mean(const std::vector<double>& xs)
{
    if (xs.empty()) {
        return std::nullopt;
    }
    const double sum = std::accumulate(xs.begin(), xs.end(), 0.0);
    return sum / static_cast<double>(xs.size());
}

std::optional<double> median(std::vector<double> xs)
{
    if (xs.empty()) {
        return std::nullopt;
    }
    std::sort(xs.begin(), xs.end());
    const std::size_t mid = xs.size() / 2;
    if (xs.size() % 2 == 1) {
        return xs[mid];
    }
    return (xs[mid - 1] + xs[mid]) / 2.0;
}

}  // namespace uni
