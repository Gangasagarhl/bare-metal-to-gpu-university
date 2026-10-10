// test_trivial.cpp: the host unit-test target of milestone P1 ("a trivial passing test").
#include <cstdio>

int main()
{
    const int expected = 4;
    const int actual = 2 + 2;
    if (actual != expected) {
        std::printf("test_trivial: FAIL (%d != %d)\n", actual, expected);
        return 1;
    }
    std::printf("test_trivial: pass\n");
    return 0;
}
