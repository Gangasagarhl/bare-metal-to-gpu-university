// F0-74 hardware note: does this compiler evaluate float expressions in a wider format?
// FLT_EVAL_METHOD 0 means: every operation is rounded to the type of its operands.
#include <cfloat>
#include <cstdio>

int main()
{
    std::printf("FLT_EVAL_METHOD = %d\n", static_cast<int>(FLT_EVAL_METHOD));
    std::printf("sizeof(float) = %zu, sizeof(double) = %zu, sizeof(long double) = %zu\n",
                sizeof(float), sizeof(double), sizeof(long double));
    std::printf("LDBL_MANT_DIG = %d (precision of long double in bits)\n", LDBL_MANT_DIG);
    return 0;
}
