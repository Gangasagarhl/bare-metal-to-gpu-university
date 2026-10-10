#include "uni/stats.h"

#include <algorithm>
#include <cstddef>
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

// Faster: nth_element only partly sorts the data.
std::optional<double> median(std::vector<double> xs)
{
    if (xs.empty()) {
        return std::nullopt;
    }
    const std::size_t mid = xs.size() / 2;
    std::nth_element(xs.begin(), xs.begin() + static_cast<std::ptrdiff_t>(mid), xs.end());
    return xs[mid];
}

}  // namespace uni
