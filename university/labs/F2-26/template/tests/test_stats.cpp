#include "uni/stats.h"
#include "uni_test.h"

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

int main(int argc, char** argv)
{
    return uni_test::run_all(argc, argv);
}
