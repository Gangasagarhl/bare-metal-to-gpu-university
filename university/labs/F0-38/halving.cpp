#include <iostream>

// Plays the guessing game against every possible secret from 1 to 100,
// always guessing the middle of the numbers that are still possible.
int guesses_needed(int secret)
{
    int low = 1;
    int high = 100;
    int tries = 0;
    while (low <= high) {
        int guess = (low + high) / 2;
        tries = tries + 1;
        if (guess == secret) {
            return tries;
        }
        if (guess < secret) {
            low = guess + 1;
        } else {
            high = guess - 1;
        }
    }
    return -1;
}

int main()
{
    int most = 0;
    int hardest = 0;
    for (int secret = 1; secret <= 100; ++secret) {
        int tries = guesses_needed(secret);
        if (tries > most) {
            most = tries;
            hardest = secret;
        }
    }
    std::cout << "Secret 22 needs " << guesses_needed(22) << " guesses.\n";
    std::cout << "Most guesses ever needed: " << most << " (first for secret " << hardest << ")\n";
    return 0;
}
