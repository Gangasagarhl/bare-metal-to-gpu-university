#include <iostream>

// Forensic evidence: each ticket must be closed exactly once at the end of service.
class Ticket
{
public:
    explicit Ticket(int table) : table_(table)
    {
        std::cout << "open ticket for table " << table_ << '\n';
    }

    ~Ticket()
    {
        std::cout << "close ticket for table " << table_ << '\n';
    }

    int table() const { return table_; }

private:
    int table_;
};

// Added last week: prints the ticket for the kitchen screen.
void show_on_screen(Ticket t)
{
    std::cout << "screen: table " << t.table() << '\n';
}

int main()
{
    Ticket t4(4);
    show_on_screen(t4);
    Ticket t9(9);
    show_on_screen(t9);
    std::cout << "end of service\n";
    return 0;
}
