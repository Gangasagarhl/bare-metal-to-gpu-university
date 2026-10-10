// kernels.cc - five small loops; the compiler's vectoriser report says which became SIMD.
#include <cstddef>

void saxpy(float* y, float const* x, float a, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        y[i] = a * x[i] + y[i];
    }
}

int sumInt(int const* x, std::size_t n)
{
    int s = 0;
    for (std::size_t i = 0; i < n; ++i) {
        s += x[i];
    }
    return s;
}

float sumFloat(float const* x, std::size_t n)
{
    float s = 0.0f;
    for (std::size_t i = 0; i < n; ++i) {
        s += x[i];  // float addition is not associative: reordering changes the result
    }
    return s;
}

void prefix(float* x, std::size_t n)
{
    for (std::size_t i = 1; i < n; ++i) {
        x[i] = x[i - 1] + x[i];  // each step needs the previous step's result
    }
}

void clampAll(float* x, std::size_t n, float lo, float hi)
{
    for (std::size_t i = 0; i < n; ++i) {
        if (x[i] < lo) {
            x[i] = lo;
        } else if (x[i] > hi) {
            x[i] = hi;
        }
    }
}
