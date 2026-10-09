// F1-23 Listing 3: one tiny C++ function, compiled for three real ISAs to compare encodings.
int add3(int a, int b, int c)
{
    return a + b + c;
}

long sum(const long* p, long n)
{
    long s = 0;
    for (long i = 0; i < n; ++i) {
        s += p[i];
    }
    return s;
}
