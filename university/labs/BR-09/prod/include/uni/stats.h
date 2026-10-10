// SPDX-License-Identifier: LicenseRef-Uni-Lab
// uni::stats, public interface of version 1 (see CHANGELOG.md and KNOWN_ISSUES.md).
// Contract, written down so callers do not have to guess:
//   * Inputs are finite numbers. A NaN or an infinity anywhere gives "no value".
//   * An empty input gives "no value".
//   * A result that does not fit in a double (overflow) gives "no value" (KI-1).
// Source compatibility promise: every call that compiled against version 0.1 still
// compiles and means the same for finite inputs (checked by the compat step in CI).
#pragma once

#include <optional>
#include <vector>

namespace uni {

// Arithmetic mean.
std::optional<double> mean(const std::vector<double>& xs);

// Middle value after sorting (midpoint of the two middle values for an even count).
// Takes a copy because it sorts.
std::optional<double> median(std::vector<double> xs);

}  // namespace uni
