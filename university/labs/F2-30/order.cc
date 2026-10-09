// order.cc: tests for the shop's price calculation. The team reports: "the CI run fails
// in price_with_default_vat, but when I run that test alone it passes".
#include "uni_test.h"

struct Settings
{
    int vat_percent = 20;
};

Settings& settings()                          // one shared object for the whole program
{
    static Settings s;
    return s;
}

int price_with_vat(int net_cents)
{
    return net_cents + net_cents * settings().vat_percent / 100;
}

UNI_TEST(price_with_reduced_vat)
{
    settings().vat_percent = 10;              // a reduced rate for this test
    CHECK(price_with_vat(1000) == 1100);
}

UNI_TEST(price_with_default_vat)
{
    CHECK(price_with_vat(1000) == 1200);
}

UNI_TEST(price_of_zero)
{
    CHECK(price_with_vat(0) == 0);
}

int main(int argc, char** argv)
{
    return uni_test::run_all(argc, argv);
}
