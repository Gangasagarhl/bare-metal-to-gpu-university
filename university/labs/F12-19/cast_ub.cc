// cast_ub.cc - F12-19 Listing 2: the direct conversion of an out-of-range double to a
// 16-bit integer. This is undefined behaviour in C++; the build in run.sh adds the
// sanitizer check -fsanitize=float-cast-overflow, which reports it at run time.
#include <cstdint>
#include <cstdio>

int main(int argc, char**)
{
    const double v = 40000.0 * argc;  // 40000 when run without arguments; not a constant
    const std::int16_t s = static_cast<std::int16_t>(v);  // undefined: 40000 > 32767
    std::printf("converted %.1f -> %d\n", v, s);
    return 0;
}
