#include <iostream>
#include <vector>

// Answer key for the forensic question of the KID102 final: forensic_table.cpp with both lines fixed
// (the loop condition k <= 10, and the score declared before the loop and printed after it).

int times(int a, int b)
{
    return a * b;
}

int check_times(int a, int b, int expected)
{
    int got = times(a, b);
    if (got == expected) {
        return 0;
    }
    std::cout << "FAIL: times(" << a << ", " << b << ") = " << got << ", expected " << expected << '\n';
    return 1;
}

int test_times()
{
    int failures = 0;
    failures = failures + check_times(7, 3, 21);
    failures = failures + check_times(0, 9, 0);
    std::cout << "test_times: " << failures << " failures\n";
    return failures;
}

void print_table(int n)
{
    for (int k = 1; k <= 10; ++k) {
        std::cout << n << " x " << k << " = " << times(n, k) << '\n';
    }
}

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
