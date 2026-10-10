// SPDX-License-Identifier: LicenseRef-Uni-Lab
#include "uni/stats.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace uni {

namespace {

bool all_finite(const std::vector<double>& xs)
{
    return std::all_of(xs.begin(), xs.end(), [](double x) { return std::isfinite(x); });
}

}  // namespace

std::optional<double> mean(const std::vector<double>& xs)
{
    if (xs.empty() || !all_finite(xs)) {
        return std::nullopt;
    }
    const double sum = std::accumulate(xs.begin(), xs.end(), 0.0);
    const double m = sum / static_cast<double>(xs.size());
    if (!std::isfinite(m)) {
        return std::nullopt;  // the sum overflowed (KNOWN_ISSUES.md, KI-1)
    }
    return m;
}

std::optional<double> median(std::vector<double> xs)
{
    // std::sort needs a strict weak ordering; NaN breaks it, so reject before sorting.
    if (xs.empty() || !all_finite(xs)) {
        return std::nullopt;
    }
    std::sort(xs.begin(), xs.end());
    const std::size_t mid = xs.size() / 2;
    if (xs.size() % 2 == 1) {
        return xs[mid];
    }
    return std::midpoint(xs[mid - 1], xs[mid]);  // no overflow for large values
}

}  // namespace uni
