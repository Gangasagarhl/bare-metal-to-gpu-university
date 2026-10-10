// Small statistics helpers used to report measurements (median of several runs).
#pragma once

#include <optional>
#include <vector>

namespace uni {

// Arithmetic mean; no value for an empty input.
std::optional<double> mean(const std::vector<double>& xs);

// Middle value after sorting (average of the two middle values for an even count);
// no value for an empty input. Takes a copy because it sorts.
std::optional<double> median(std::vector<double> xs);

}  // namespace uni
