// narrowing.cc - F11-11: why "warnings as errors" with -Wconversion and friends
// is part of a coding standard. A device length field is 32-bit unsigned; here it
// is quietly stored in a signed short, which both changes its value and can go
// negative. -Wconversion and -Wsign-conversion flag it at compile time. run.sh
// compiles this once WITHOUT the extra warnings (it builds) and once WITH
// -Wconversion -Wsign-conversion -Werror (it fails to build): the failing build
// is the lesson.
#include <cstdio>
#include <cstdint>

static short shrink(std::uint32_t device_length)
{
    short n = device_length;          // narrowing: 32-bit unsigned -> 16-bit signed
    return n;
}

int main()
{
    std::uint32_t from_device = 70000;        // a legitimate 32-bit value
    short n = shrink(from_device);
    std::printf("device said %u, we stored %d\n", from_device, n);  // prints a wrong, maybe negative, number
    return 0;
}
