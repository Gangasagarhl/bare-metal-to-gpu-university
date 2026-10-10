// BR-01 Listing 1: the CPU program we start from. Plain C++, built with g++.
#include <cstddef>
#include <cstdio>
#include <vector>

template <typename T>
T add(T a, T b)
{
    return a + b;
}

void vecAddCpu(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c)
{
    for (std::size_t i = 0; i < c.size(); ++i) {   // one loop visits every element in turn
        c[i] = add(a[i], b[i]);
    }
}

int main()
{
    const std::size_t n = 1000003;                  // deliberately not a round number
    std::vector<float> a(n), b(n), c(n);
    for (std::size_t i = 0; i < n; ++i) {
        a[i] = 1.0f * static_cast<float>(i);
        b[i] = 2.0f * static_cast<float>(i);
    }
    vecAddCpu(a, b, c);
    std::size_t errors = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (c[i] != 3.0f * static_cast<float>(i)) {  // exact: every value fits a float
            ++errors;
        }
    }
    std::printf("n = %zu, c[1] = %.1f, c[n-1] = %.1f, errors = %zu\n", n, c[1], c[n - 1], errors);
    return errors == 0 ? 0 : 1;
}
