// F1-31 Listing 1: three small functions to read in assembly, compiled by run.sh for
// x86-64, ARM64 and RISC-V at several optimisation levels.
int add(int a, int b)
{
    return a + b;
}

int maxOf(int a, int b)
{
    if (a > b) {
        return a;
    }
    return b;
}

int sumArray(const int* p, int n)
{
    int s = 0;
    for (int i = 0; i < n; ++i) {
        s += p[i];
    }
    return s;
}
