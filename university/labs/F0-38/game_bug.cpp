#include <iostream>
#include <random>
#include <string>

// Picks the secret number between 1 and 100.
// The same seed always gives the same secret with the same toolchain.
int pick_secret(unsigned int seed)
{
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> one_to_hundred(1, 100);
    return one_to_hundred(generator);
}

// Compares a guess with the secret and returns the hint to show.
std::string hint_for(int guess, int secret)
{
    if (guess > secret) {
        return "Too low.";
    } else if (guess < secret) {
        return "Too high.";
    }
    return "Correct!";
}

// Asks for guesses until the player is right or the input runs out.
// Returns the number of guesses used, or 0 if the player never got it.
int play(int secret)
{
    int tries = 0;
    int guess = 0;
    std::cout << "I am thinking of a number from 1 to 100.\n";
    while (std::cin >> guess) {
        tries = tries + 1;
        std::string hint = hint_for(guess, secret);
        std::cout << "Guess " << tries << ": " << guess << " -> " << hint << '\n';
        if (hint == "Correct!") {
            return tries;
        }
    }
    std::cout << "No more guesses. The number was " << secret << ".\n";
    return 0;
}

// Checks one answer of hint_for. Returns 0 if it is right and 1 if it is wrong.
int check_hint(int guess, int secret, std::string expected)
{
    std::string got = hint_for(guess, secret);
    if (got == expected) {
        return 0;
    }
    std::cout << "FAIL: hint_for(" << guess << ", " << secret << ") gave \"" << got
              << "\", expected \"" << expected << "\"\n";
    return 1;
}

// The test function: answers worked out by hand. Returns the number of failures.
int test_hint_for()
{
    int failures = 0;
    failures = failures + check_hint(10, 50, "Too low.");
    failures = failures + check_hint(90, 50, "Too high.");
    failures = failures + check_hint(50, 50, "Correct!");
    failures = failures + check_hint(1, 1, "Correct!");
    std::cout << "test_hint_for: " << failures << " failures\n";
    return failures;
}

int main()
{
    test_hint_for();  // "I will look at the test results later."
    const unsigned int seed = 2026;  // fixed so that the recorded run can be repeated
    int secret = pick_secret(seed);
    int tries = play(secret);
    if (tries > 0) {
        std::cout << "You found it in " << tries << " guesses.\n";
    }
    return 0;
}
