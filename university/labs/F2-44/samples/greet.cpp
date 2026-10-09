// greet.cpp: a tiny shared library (built with -fPIC -shared) for the lab's fifth binary.
extern "C" int greet_twice(int x)
{
    return 2 * x;
}
