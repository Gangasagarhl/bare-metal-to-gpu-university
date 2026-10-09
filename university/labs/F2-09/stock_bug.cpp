#include <iostream>
#include <string>

// Forensic evidence: the Stock "class" after a quick change on a busy evening.
class Stock
{
public:
    Stock(std::string name, int portions, int capacity)
        : name(name), portions(portions), capacity(capacity)
    {
    }

    bool take(int n)
    {
        if (n < 0 || n > portions) {
            return false;
        }
        portions = portions - n;
        return true;
    }

    std::string name;  // moved to public "to make the party menu easier"
    int portions;
    int capacity;
};

void log_line(const std::string& when, const Stock& s)
{
    std::cout << when << "  " << s.name << " portions=" << s.portions << '\n';
}

// Added for the party: serves one portion per guest.
void serve_party(Stock& s, int guests)
{
    s.portions = s.portions - guests;
}

int main()
{
    Stock soup("soup", 12, 20);
    log_line("18:00", soup);
    std::cout << "18:10  take(5) " << (soup.take(5) ? "done" : "refused") << '\n';
    log_line("18:10", soup);
    serve_party(soup, 9);
    std::cout << "18:30  party of 9 served\n";
    log_line("18:30", soup);
    std::cout << "18:45  take(1) " << (soup.take(1) ? "done" : "refused") << '\n';
    log_line("18:45", soup);
    return 0;
}
