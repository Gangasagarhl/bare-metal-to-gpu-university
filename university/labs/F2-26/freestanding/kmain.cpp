// A freestanding translation unit: it uses no C or C++ library at all, so it can be
// linked into a kernel later (curriculum P1). Built with -ffreestanding -fno-exceptions -fno-rtti.
extern "C" int kmain(int boot_value)
{
    int sum = 0;
    for (int i = 0; i < boot_value; ++i) {
        sum += i;
    }
    return sum;
}
