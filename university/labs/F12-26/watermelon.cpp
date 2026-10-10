// watermelon.cpp - compare the colour a status report showed with the colour its
// own data supports. Input lines:
//   week <n> <reported colour> <planned %> <earned %> | <headline sentence>
// Data colour (this course's convention, F12-26; the team may agree another one
// in advance): ratio = earned / planned; green if >= 0.90, amber if >= 0.75, else red.
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::string dataColour(double planned, double earned)
{
    if (planned <= 0) {
        return "green";
    }
    const double r = earned / planned;
    return r >= 0.90 ? "green" : (r >= 0.75 ? "amber" : "red");
}

int rank(const std::string& c)
{
    return c == "green" ? 0 : (c == "amber" ? 1 : 2);
}

int main()
{
    std::string line;
    int firstGap = 0, gapWeeks = 0;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "week  reported  planned  earned  ratio  data    gap  headline\n";
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind, colour, bar;
        int n = 0;
        double planned = 0, earned = 0;
        if (!(in >> kind) || kind[0] == '#') {
            continue;
        }
        if (kind != "week" || !(in >> n >> colour >> planned >> earned >> bar) || bar != "|") {
            std::cout << "cannot read: " << line << '\n';
            return 2;
        }
        std::string headline;
        std::getline(in, headline);
        headline.erase(0, headline.find_first_not_of(' '));
        const std::string data = dataColour(planned, earned);
        const int gap = rank(data) - rank(colour);  // > 0: the report is rosier than the data
        if (gap > 0) {
            ++gapWeeks;
            if (firstGap == 0) {
                firstGap = n;
            }
        }
        std::cout << std::setw(4) << n << "  " << std::left << std::setw(8) << colour << std::right
                  << std::setw(9) << std::setprecision(1) << planned << std::setw(8) << earned
                  << std::setw(7) << std::setprecision(2) << (planned > 0 ? earned / planned : 1.0)
                  << "  " << std::left << std::setw(6) << data << std::right << "  "
                  << (gap > 0 ? std::string(static_cast<std::size_t>(gap), '+') : std::string("-"))
                  << std::string(gap > 0 ? 4 - gap : 3, ' ') << headline << '\n';
    }
    std::cout << "\nweeks where the report was rosier than its data: " << gapWeeks << '\n';
    if (firstGap != 0) {
        std::cout << "first such week: " << firstGap << '\n';
    }
    return 0;
}
