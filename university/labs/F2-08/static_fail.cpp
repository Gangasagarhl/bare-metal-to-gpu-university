#include <iostream>

constexpr int grams_per_portion = 0;     // someone set this to 0 by mistake
static_assert(grams_per_portion > 0, "a portion must weigh something");

int main()
{
    std::cout << 1000 / grams_per_portion << " portions per kilogram\n";
    return 0;
}
