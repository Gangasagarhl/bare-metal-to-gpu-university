#include <iostream>
#include <vector>

// KID102 practical exam: STARTING FILE. Complete the TODO parts.
// Builds and runs as given: the test function reports failures and main stops with exit code 1.

// TODO 1: make times return a multiplied by b. (It adds on purpose, so the file builds
// and the test function shows you what a failing test looks like.)
int times(int a, int b)
{
    return a + b;
}

// Checks one answer of times. Returns 0 when it is right and 1 when it is wrong.
int check_times(int a, int b, int expected)
{
    int got = times(a, b);
    if (got == expected) {
        return 0;
    }
    std::cout << "FAIL: times(" << a << ", " << b << ") = " << got << ", expected " << expected << '\n';
    return 1;
}

// TODO 2: add at least one more case worked out by hand, including an edge case (0 or 1).
int test_times()
{
    int failures = 0;
    failures = failures + check_times(7, 3, 21);
    failures = failures + check_times(12, 10, 120);
    std::cout << "test_times: " << failures << " failures\n";
    return failures;
}

// TODO 3: print the table of n from n x 1 to n x 10, one line each, like "7 x 3 = 21".
void print_table(int n)
{
    std::cout << "(table of " << n << " not written yet)\n";
}

// TODO 4: ask "What is n x k? ", read the answer, print "Right!" or "Wrong, it is 28." and
// return 1 for right, 0 for wrong, -1 when no answer could be read.
int ask(int n, int k)
{
    std::cout << "(question " << n << " x " << k << " not written yet)\n";
    return 0;
}

int main()
{
    if (test_times() != 0) {
        return 1;
    }
    int n = 0;
    std::cout << "Which table do you want to practise (1 to 12)? ";
    std::cin >> n;
    std::cout << '\n';
    // TODO 5: refuse a number outside 1 to 12 with the message "Please choose a number from 1 to 12."
    print_table(n);

    const std::vector<int> multipliers = {4, 6, 9};
    int score = 0;
    for (int k : multipliers) {
        int result = ask(n, k);
        if (result < 0) {
            break;
        }
        score = score + result;
    }
    std::cout << "Score: " << score << " out of " << multipliers.size() << '\n';
    return 0;
}
