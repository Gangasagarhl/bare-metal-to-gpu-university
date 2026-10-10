// show_rows.cpp - prints the evidence pack excerpt: the column header and the rows of
// each log at chosen times (every 10 s, every 0.5 s around the last 6 s), unchanged.
#include <cstdio>
#include <fstream>
#include <string>

void show(const char* file)
{
    std::ifstream in(file);
    std::string line, last_t;
    std::printf("---- %s (excerpt; '#' lines are shown by logscan)\n", file);
    double t_end = 0;
    {
        std::ifstream scan(file);
        std::string l;
        while (std::getline(scan, l))
            if (!l.empty() && l[0] != '#' && l[0] != 't')
                t_end = std::stod(l.substr(0, l.find(',')));
    }
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == 't') {
            std::printf("%s\n", line.c_str());
            continue;
        }
        const double t = std::stod(line.substr(0, line.find(',')));
        const long ms = static_cast<long>(t * 1000 + 0.5);
        if (ms % 10000 == 0 || t > t_end - 6.0) std::printf("%s\n", line.c_str());
    }
}

int main()
{
    show("log_a_flyaway.csv");
    show("log_b_late_battery.csv");
    return 0;
}
