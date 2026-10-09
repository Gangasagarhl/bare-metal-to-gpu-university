// F0-61 Listing 1: Bayes' rule two ways for a fault detector (exercise numbers):
// with probabilities, and with "natural frequencies" (counts out of 10000 parts).
#include <iostream>

int main()
{
    const double prior = 0.01;          // P(faulty): 1 part in 100 is faulty
    const double sensitivity = 0.95;    // P(alarm | faulty)
    const double falseAlarm = 0.05;     // P(alarm | good)

    // Law of total probability: all the ways an alarm can happen.
    const double pAlarm = sensitivity * prior + falseAlarm * (1.0 - prior);
    // Bayes' rule.
    const double posterior = sensitivity * prior / pAlarm;
    const double pFaultyGivenQuiet = (1.0 - sensitivity) * prior / (1.0 - pAlarm);

    std::cout << "P(alarm) = " << sensitivity << " * " << prior << " + " << falseAlarm
              << " * " << 1.0 - prior << " = " << pAlarm << "\n";
    std::cout << "P(faulty | alarm) = " << sensitivity * prior << " / " << pAlarm << " = "
              << posterior << "\n";
    std::cout << "P(faulty | no alarm) = " << pFaultyGivenQuiet << "\n\n";

    const int parts = 10000;
    const int faulty = static_cast<int>(parts * prior + 0.5);
    const int good = parts - faulty;
    const int trueAlarms = static_cast<int>(faulty * sensitivity + 0.5);
    const int falseAlarms = static_cast<int>(good * falseAlarm + 0.5);
    std::cout << "out of " << parts << " parts: " << faulty << " faulty, " << good << " good\n";
    std::cout << "alarms on faulty parts: " << trueAlarms << ", alarms on good parts: "
              << falseAlarms << "\n";
    std::cout << "share of alarms that are real: " << trueAlarms << " / "
              << trueAlarms + falseAlarms << " = "
              << static_cast<double>(trueAlarms) / (trueAlarms + falseAlarms) << "\n";
    return 0;
}
