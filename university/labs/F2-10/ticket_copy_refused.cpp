class Ticket
{
public:
    explicit Ticket(int table) : table_(table) {}
    Ticket(const Ticket&) = delete;
    Ticket& operator=(const Ticket&) = delete;
    int table() const { return table_; }

private:
    int table_;
};

int show_on_screen(Ticket t)  // the old signature: takes the ticket by value
{
    return t.table();
}

int main()
{
    Ticket t4(4);
    return show_on_screen(t4);
}
