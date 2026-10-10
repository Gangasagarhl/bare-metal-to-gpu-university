#include <iostream>
#include <stdexcept>

// Lab reference solution: a restaurant table.
// Invariant: 1 <= seats_ and 0 <= guests_ <= seats_.
class Table
{
public:
    Table(int number, int seats) : number_(number), seats_(seats), guests_(0)
    {
        if (seats_ < 1) {
            throw std::invalid_argument("Table: a table needs at least one seat");
        }
    }

    bool seat(int n)
    {
        if (n < 1 || n > seats_) {  // lab step 5: broken on purpose
            return false;
        }
        guests_ = guests_ + n;
        return true;
    }

    bool leave(int n)
    {
        if (n < 1 || n > guests_) {
            return false;
        }
        guests_ = guests_ - n;
        return true;
    }

    int number() const { return number_; }
    int free_seats() const { return seats_ - guests_; }
    bool invariant_holds() const { return seats_ >= 1 && guests_ >= 0 && guests_ <= seats_; }

private:
    int number_;
    int seats_;
    int guests_;
};

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

int main()
{
    Table t(4, 6);
    expect(t.free_seats() == 6, "new table has 6 free seats");
    expect(t.seat(4), "seat 4 at a table of 6");
    expect(!t.seat(3), "refuse 3 more when only 2 seats are free");
    expect(t.free_seats() == 2, "refused request changed nothing");
    expect(!t.seat(0), "refuse a group of 0");
    expect(!t.leave(5), "refuse 5 leaving when 4 are seated");
    expect(t.leave(4), "4 guests leave");
    expect(t.invariant_holds(), "invariant holds at the end");

    bool refused = false;
    try {
        Table broken(9, 0);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    expect(refused, "constructor refuses a table with 0 seats");

    std::cout << "table tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
