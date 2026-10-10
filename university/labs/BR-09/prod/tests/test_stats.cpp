// SPDX-License-Identifier: LicenseRef-Uni-Lab
// Unit tests of uni::mean and uni::median. The first five tests are version 0.1's,
// unchanged (compatibility); the rest test the failure paths added in 1.0.0.
#include "uni/stats.h"
#include "uni_test.h"

#include <limits>

UNI_TEST(mean_of_three)
{
    const auto m = uni::mean({1.0, 2.0, 6.0});
    CHECK(m.has_value());
    CHECK_NEAR(*m, 3.0, 1e-12);
}

UNI_TEST(mean_of_empty_has_no_value)
{
    CHECK(!uni::mean({}).has_value());
}

UNI_TEST(median_odd_count_is_middle_value)
{
    CHECK_NEAR(*uni::median({9.0, 1.0, 5.0}), 5.0, 0.0);
}

UNI_TEST(median_even_count_averages_middle_pair)
{
    CHECK_NEAR(*uni::median({4.0, 1.0, 3.0, 2.0}), 2.5, 0.0);
}

UNI_TEST(median_of_empty_has_no_value)
{
    CHECK(!uni::median({}).has_value());
}

// ---- failure paths (new in 1.0.0) ---------------------------------------------------

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kMax = std::numeric_limits<double>::max();

UNI_TEST(nan_anywhere_gives_no_value)
{
    // Version 0.1 printed median 1, nan or 2 depending on where the NaN was.
    CHECK(!uni::median({kNaN, 1.0, 2.0}).has_value());
    CHECK(!uni::median({1.0, kNaN, 2.0}).has_value());
    CHECK(!uni::median({1.0, 2.0, kNaN}).has_value());
    CHECK(!uni::mean({1.0, kNaN}).has_value());
}

UNI_TEST(infinity_gives_no_value)
{
    CHECK(!uni::mean({kInf, 1.0}).has_value());
    CHECK(!uni::median({1.0, -kInf}).has_value());
}

UNI_TEST(median_of_two_huge_values_does_not_overflow)
{
    const auto md = uni::median({kMax, kMax});
    CHECK(md.has_value());
    CHECK(md && *md == kMax);
}

UNI_TEST(known_issue_ki1_mean_of_two_huge_values_has_no_value)
{
    // Pins today's behaviour of KI-1: the sum overflows although the mean would fit.
    // If this test starts failing, KI-1 was fixed: update KNOWN_ISSUES.md and CHANGELOG.md.
    CHECK(!uni::mean({kMax, kMax}).has_value());
}

UNI_TEST(single_value_is_its_own_mean_and_median)
{
    CHECK_NEAR(*uni::mean({-7.5}), -7.5, 0.0);
    CHECK_NEAR(*uni::median({-7.5}), -7.5, 0.0);
}

int main(int argc, char** argv)
{
    return uni_test::run_all(argc, argv);
}
