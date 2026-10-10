// test_duration.cpp: unit tests for parse_duration, written with uni_test.h.
#include "duration.h"
#include "uni_test.h"

UNI_TEST(single_units)
{
    CHECK(parse_duration("45s") == 45);
    CHECK(parse_duration("2m") == 120);
    CHECK(parse_duration("1h") == 3600);
}

UNI_TEST(combined_units)
{
    CHECK(parse_duration("1h30m") == 5400);
    CHECK(parse_duration("2m5s") == 125);
    CHECK(parse_duration("1h0m1s") == 3601);
}

UNI_TEST(boundaries)
{
    CHECK(parse_duration("0s") == 0);
    CHECK(parse_duration("90m") == 5400);     // no upper limit per unit
}

UNI_TEST(invalid_inputs)
{
    CHECK(!parse_duration(""));
    CHECK(!parse_duration("5"));              // number without unit
    CHECK(!parse_duration("m"));              // unit without number
    CHECK(!parse_duration("5x"));             // unknown unit
    CHECK(!parse_duration("1m1m"));           // repeated unit
    CHECK(!parse_duration("5s1m"));           // out of order
    CHECK(!parse_duration("-5s"));            // sign not allowed
}

int main(int argc, char** argv)
{
    return uni_test::run_all(argc, argv);
}
