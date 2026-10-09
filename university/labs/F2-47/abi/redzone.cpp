// redzone.cpp: a leaf function with a local array. Compiled twice: with the default
// System V options and with -mno-red-zone (what kernels use).
int leafWithArray(int seed)
{
    volatile int scratch[8];   // volatile: the stores must really happen
    for (int i = 0; i < 8; ++i) {
        scratch[i] = seed + i;
    }
    return scratch[3] + scratch[5];
}
