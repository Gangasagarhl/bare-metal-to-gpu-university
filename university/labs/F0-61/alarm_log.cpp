// F0-61 forensic evidence: one day of the inspection station's log, summarised.
// Each part is faulty or good; the camera check raises an alarm or stays quiet.
#include <iostream>
#include <random>

double uniform01(std::mt19937& engine)
{
    return (static_cast<double>(engine()) + 0.5) / 4294967296.0;
}

int main()
{
    std::mt19937 engine(61);
    const int parts = 20000;
    int faultyAlarm = 0;
    int faultyQuiet = 0;
    int goodAlarm = 0;
    int goodQuiet = 0;
    for (int i = 0; i < parts; ++i) {
        const bool faulty = uniform01(engine) < 0.005;
        const bool alarm = faulty ? uniform01(engine) < 0.98 : uniform01(engine) < 0.03;
        if (faulty) {
            ++(alarm ? faultyAlarm : faultyQuiet);
        } else {
            ++(alarm ? goodAlarm : goodQuiet);
        }
    }
    std::cout << "parts inspected today: " << parts << "\n";
    std::cout << "                 alarm    quiet\n";
    std::cout << "faulty part      " << faultyAlarm << "       " << faultyQuiet << "\n";
    std::cout << "good part        " << goodAlarm << "      " << goodQuiet << "\n";
    std::cout << "vendor sheet: detects 98 % of faulty parts, alarms on 3 % of good parts\n";
    return 0;
}
