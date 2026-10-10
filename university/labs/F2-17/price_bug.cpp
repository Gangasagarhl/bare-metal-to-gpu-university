#include <iostream>
#include <string>
#include <vector>

// Forensic evidence: the till. The bill function is made once, when the restaurant opens.
struct Sale
{
    std::string time;
    int portions;
};

int main()
{
    int soup_price = 5;   // price when the restaurant opens

    auto bill = [soup_price](int portions) { return portions * soup_price; };

    std::vector<Sale> sales = {{"12:10", 2}, {"13:30", 1}, {"18:05", 3}, {"19:40", 2}};
    for (const Sale& s : sales) {
        if (s.time == "18:05") {
            soup_price = 6;   // the evening price starts at 18:00
            std::cout << "18:00 price changed to " << soup_price << '\n';
        }
        std::cout << s.time << "  " << s.portions << " soup -> bill " << bill(s.portions) << '\n';
    }
    return 0;
}
