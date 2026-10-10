// saxpy.cc: two loops for the compilers' optimisation reports.
#include <cstddef>

void saxpy(float a, const float* x, float* y, std::size_t n)
{
    for (std::size_t i = 0; i < n; ++i) {
        y[i] = a * x[i] + y[i];               // each step is independent of the others
    }
}

void running_sum(float* y, std::size_t n)
{
    for (std::size_t i = 1; i < n; ++i) {
        y[i] = y[i] + y[i - 1];               // each step needs the previous result
    }
}
