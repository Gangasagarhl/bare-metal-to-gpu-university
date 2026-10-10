// abi_demo.cpp: small functions whose compiled code shows the calling convention.
// Compiled (not run) for four targets in run.sh; the disassembly is the evidence.
struct Pair {           // 16 bytes
    long x, y;
};
struct Big {            // 32 bytes
    long v[4];
};

long sum8(long a, long b, long c, long d, long e, long f, long g, long h)
{
    return a + b + c + d + e + f + g + h;
}

long pairSum(Pair p)    // a small struct passed by value
{
    return p.x + p.y;
}

Big makeBig(long seed)  // a large struct returned by value
{
    return Big{{seed, seed + 1, seed + 2, seed + 3}};
}

double mix(int a, double b, int c, double d)
{
    return a * b + c * d;
}

long external(long x);  // defined in another file: the compiler must emit a real call

long callsOut(long x)   // a non-leaf function: it calls another function
{
    return external(x) + 1;
}
