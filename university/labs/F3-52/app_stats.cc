// app_stats.cc - F3-52 forensic: the application that reported a wrong total.
// It has its own form-checking helper, written long before it started using libstats.
#include <cstdio>

extern "C" void stats_add(int v);
extern "C" int stats_total();

extern "C" int validate(int field)      // 1 if a form field is filled in, else 0
{
    return field != 0 ? 1 : 0;
}

int main()
{
    const int values[] = {5, 7, -3};
    int filled = 0;
    for (int v : values) {
        filled += validate(v);
        stats_add(v);
    }
    std::printf("fields filled: %d\n", filled);
    std::printf("stats_total() of 5, 7, -3 (negatives ignored) = %d (expected 12)\n", stats_total());
    return 0;
}
