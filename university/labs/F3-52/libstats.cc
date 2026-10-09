// libstats.cc - F3-52 forensic: a statistics library from the port's ports tree.
// API: stats_add(value), stats_total(). Negative values are ignored by design.
#define STATS_API __attribute__((visibility("default")))

extern "C" int validate(int v)          // helper: negative inputs count as 0
{
    return v < 0 ? 0 : v;
}

static int total = 0;

extern "C" STATS_API void stats_add(int v)
{
    total += validate(v);
}

extern "C" STATS_API int stats_total()
{
    return total;
}
