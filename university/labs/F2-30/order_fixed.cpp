// order_fixed.cpp: the same tests after the forensic lab. Each test that changes the
// shared settings restores them, with a small RAII guard, so the order no longer matters.
#include "uni_test.h"

struct Settings
{
    int vat_percent = 20;
};

Settings& settings()
{
    static Settings s;
    return s;
}

int price_with_vat(int net_cents)
{
    return net_cents + net_cents * settings().vat_percent / 100;
}

class SettingsGuard                           // saves the settings, restores them on exit
{
public:
    SettingsGuard() : saved_(settings()) {}
    ~SettingsGuard() { settings() = saved_; }
    SettingsGuard(const SettingsGuard&) = delete;
    SettingsGuard& operator=(const SettingsGuard&) = delete;

private:
    Settings saved_;
};

UNI_TEST(price_with_reduced_vat)
{
    const SettingsGuard guard;
    settings().vat_percent = 10;
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
