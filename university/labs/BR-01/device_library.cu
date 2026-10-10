// BR-01 Listing 6 (does not compile, on purpose): which familiar C++ pieces does device code
// accept? Four lines in one kernel; the compiler's messages say which ones it refused.
#include <algorithm>
#include <cmath>
#include <cstdio>

constexpr float twice(float x)                      // an ordinary constexpr function
{
    return 2.0f * x;
}

__global__ void probe(float* p)
{
    p[0] = std::sqrt(p[0]);                         // line 14: a <cmath> function
    p[1] = std::max(p[1], 1.0f);                    // line 15: an <algorithm> function
    p[2] = twice(p[2]);                             // line 16: our own constexpr function
    printf("thread %u done\n", threadIdx.x);        // line 17: printf from <cstdio>
}

int main()
{
    return 0;
}
