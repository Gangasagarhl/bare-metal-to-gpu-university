// F0-75 Listing 2: Kahan summation rescues small terms that plain float addition throws away.
#include <cstdio>
#include <vector>

#include "data.hpp"

int main()
{
    std::vector<float> x(1000001, 1.0e-8f);
    x[0] = 1.0f; // one big value, then a million tiny ones: exact sum = 1 + 0.01 = 1.01
    std::printf("exact (to float precision)  1.01\n");
    std::printf("forward float               %.9g\n", static_cast<double>(sumForward(x)));
    std::printf("pairwise float              %.9g\n",
                static_cast<double>(sumPairwise(x.data(), x.size())));
    std::printf("Kahan float                 %.9g\n", static_cast<double>(sumKahan(x)));
    std::printf("reference (Kahan in double) %.9g\n", referenceSum(x));
    return 0;
}
