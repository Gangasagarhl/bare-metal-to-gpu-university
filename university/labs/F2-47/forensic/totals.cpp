// totals.cpp: adds up scale(v[i]) for a small table. Expected total for {1..8}: 116.
#include <cstdio>

extern "C" long scale(long x);

__attribute__((noinline)) long total(const long* v, int n)
{
    long sum = 0;
    for (int i = 0; i < n; ++i) {
        sum += scale(v[i]);
    }
    return sum;
}

int main()
{
    const long v[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    const long t = total(v, 8);
    std::printf("total = %ld (expected 116)\n", t);
    return t == 116 ? 0 : 1;
}
