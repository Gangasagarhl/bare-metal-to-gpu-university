// F0-61 checks: recompute every number used in the chapter text.
#include <iostream>

double bayes(double prior, double likeH, double likeNotH)
{
    return likeH * prior / (likeH * prior + likeNotH * (1.0 - prior));
}

int main()
{
    // The dog: P(visitor) = 0.1 per ten-minute stretch; barks 0.9 if visitor, 0.2 if not.
    const double afterBark = bayes(0.1, 0.9, 0.2);
    std::cout << "P(bark) = " << 0.9 * 0.1 + 0.2 * 0.9 << ", P(visitor | bark) = " << afterBark
              << "\n";
    std::cout << "odds form: prior odds 1:9, likelihood ratio " << 0.9 / 0.2
              << ", posterior odds " << (0.1 / 0.9) * (0.9 / 0.2) << " (1:2)\n";
    // Then the doorbell rings: 0.7 if visitor, 0.01 if not.
    std::cout << "then bell: P(visitor | bark, bell) = " << bayes(afterBark, 0.7, 0.01) << "\n";
    std::cout << "bell alone from the prior: " << bayes(0.1, 0.7, 0.01) << "\n";
    // Fault detector with a higher fault rate.
    std::cout << "detector with prior 0.10: P(faulty | alarm) = " << bayes(0.10, 0.95, 0.05)
              << "\n";
    // Two independent alarms in a row (prior 0.01).
    const double one = bayes(0.01, 0.95, 0.05);
    std::cout << "after one alarm " << one << ", after a second independent alarm "
              << bayes(one, 0.95, 0.05) << "\n";
    // Forensic: 108 of 745 alarms real.
    std::cout << "forensic: real share of alarms = 108 / " << 108 + 637 << " = "
              << 108.0 / (108 + 637) << "\n";
    std::cout << "forensic expected: " << bayes(0.005, 0.98, 0.03) << ", with 5 % faulty: "
              << bayes(0.05, 0.98, 0.03) << "\n";
    std::cout << "good parts thrown away per day: 637 of " << 637 + 19254 << " = "
              << 637.0 / (637 + 19254) << "\n";
    // Check yourself: rain example.
    std::cout << "umbrella: P(rain | umbrella) = " << bayes(0.3, 0.8, 0.1) << "\n";
    std::cout << "door filter, one 'closed' reading from 0.5: " << bayes(0.5, 0.4, 0.8) << "\n";
    return 0;
}
