// F6-33 Listing 4: is x / 3 the same as x * (1 / 3) in float? Checked on the host
// (IEEE-754 single precision, round to nearest) for the forensic program's inputs.
#include <cstdio>

int main()
{
    const float s = 3.0f;
    const float inv = 1.0f / s;                           // rounded once
    int differ = 0;
    int first = -1;
    for (int i = 0; i < 1000; ++i) {                      // the values i % 1000 of normalize.cu
        const float x = static_cast<float>(i);
        const float q = x / s;                            // correctly rounded quotient
        const float m = x * inv;                          // product with a rounded reciprocal
        if (q != m) {
            ++differ;
            if (first < 0) {
                first = i;
            }
        }
    }
    std::printf("inputs 0..999: %d of 1000 differ between x / 3 and x * (1 / 3)\n", differ);
    if (first >= 0) {
        const float x = static_cast<float>(first);
        std::printf("first difference at x = %d: %.9g versus %.9g\n", first, x / s, x * inv);
    }
    return 0;
}
