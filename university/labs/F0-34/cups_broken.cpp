#include <iostream>

// Each guest drinks 2 cups of juice, and we keep 1 extra cup just in case.
int cups_needed(int guests)
{
    return guests * 2;
}

// Returns 0 when the answer is right and 1 when it is wrong.
int check(int guests, int expected)
{
    int got = cups_needed(guests);
    if (got == expected) {
        std::cout << "PASS: ";
    } else {
        std::cout << "FAIL: ";
    }
    std::cout << "cups_needed(" << guests << ") = " << got << ", expected " << expected << '\n';
    if (got == expected) {
        return 0;
    }
    return 1;
}

int test_cups_needed()
{
    int failures = 0;
    failures = failures + check(0, 1);
    failures = failures + check(1, 3);
    failures = failures + check(4, 9);
    return failures;
}

int main()
{
    if (test_cups_needed() > 0) {
        std::cout << "Some tests failed.\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    std::cout << "For a party of 6 we need " << cups_needed(6) << " cups.\n";
    return 0;
}
