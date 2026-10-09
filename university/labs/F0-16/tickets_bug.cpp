// Forensic evidence for F0-16: helper tickets made with one wrong formula (deliberate).
#include <iostream>

int main()
{
    int tables = 3;
    int seatsPerTable = 4;

    std::cout << "Helper tickets for 3 tables of 4 seats (12 helpers)\n";
    std::cout << "rule used: ticket = seat x seatsPerTable + table\n";
    int highest = 0;
    for (int table = 0; table < tables; ++table) {
        std::cout << "table " << table << ":";
        for (int seat = 0; seat < seatsPerTable; ++seat) {
            int ticket = seat * seatsPerTable + table;
            std::cout << " " << ticket;
            if (ticket > highest) {
                highest = ticket;
            }
        }
        std::cout << "\n";
    }
    std::cout << "highest ticket: " << highest << "\n";
    return 0;
}
