#include <iostream>

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

    Ticket(const Ticket&) = delete;             // a ticket must never be copied
    Ticket& operator=(const Ticket&) = delete;

    int table() const { return table_; }

private:
    int table_;
};

void show_on_screen(const Ticket& t)  // look at the ticket, do not copy it
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
