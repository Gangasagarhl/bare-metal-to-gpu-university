#include <iostream>
#include <vector>

// KID102 practical exam: reference solution, identical to times_table.cpp; run with the input in times_table_range.in (a table outside 1 to 12).
// Multiplies two whole numbers. This is the function under test.
int times(int a, int b)
{
    return a * b;
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

// The test function: answers worked out by hand, including the edge case 0.
int test_times()
{
    int failures = 0;
    failures = failures + check_times(7, 3, 21);
    failures = failures + check_times(12, 10, 120);
    failures = failures + check_times(0, 9, 0);
    failures = failures + check_times(1, 1, 1);
    std::cout << "test_times: " << failures << " failures\n";
    return failures;
}

// Prints the table of n from n x 1 to n x 10, one line each.
void print_table(int n)
{
    for (int k = 1; k <= 10; ++k) {
        std::cout << n << " x " << k << " = " << times(n, k) << '\n';
    }
}

// Asks one question and reads the answer. Returns 1 for a right answer, 0 for a wrong one,
// and -1 when no answer could be read (the input ended).
int ask(int n, int k)
{
    int answer = 0;
    std::cout << "What is " << n << " x " << k << "? ";
    if (!(std::cin >> answer)) {
        std::cout << "\nThe input ended early.\n";
        return -1;
    }
    std::cout << '\n';
    if (answer == times(n, k)) {
        std::cout << "Right!\n";
        return 1;
    }
    std::cout << "Wrong, it is " << times(n, k) << ".\n";
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
    if (n < 1 || n > 12) {
        std::cout << "Please choose a number from 1 to 12.\n";
        return 0;
    }
    print_table(n);

    // The quiz asks three fixed questions (4, 6 and 9) so that a recorded run can be repeated.
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
