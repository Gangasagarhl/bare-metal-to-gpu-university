// F7-18 Listing 3: an impossible instance is rejected when it is compiled, not when it runs.
#include "mini_ck.hpp"
#include <cstdio>

int main()
{
    using Bad = mini::GemmPolicy<64, 40, 16, 4, 4, true>;   // (64/4) * (40/4) = 160 threads
    std::printf("%d\n", Bad::threads);
    return 0;
}
