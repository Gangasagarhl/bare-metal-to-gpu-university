// F6-04 Listing 1: the CPU version you start from (the "before" of the port).
#include <cstdio>
#include <vector>

void vecAddCpu(const std::vector<float>& a, const std::vector<float>& b, std::vector<float>& c)
{
    for (std::size_t i = 0; i < a.size(); ++i) {   // one loop does every element in turn
        c[i] = a[i] + b[i];
    }
}

int main()
{
    const std::size_t n = 1000;
    std::vector<float> a(n), b(n), c(n);
    for (std::size_t i = 0; i < n; ++i) { a[i] = 1.0f * i; b[i] = 2.0f * i; }
    vecAddCpu(a, b, c);
    std::printf("c[0] = %.1f, c[1] = %.1f, c[999] = %.1f\n", c[0], c[1], c[999]);
    return 0;
}
