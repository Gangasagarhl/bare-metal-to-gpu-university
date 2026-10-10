#include <iostream>
#include <stdexcept>
#include <string>

// A shelf of one dish in the kitchen.
// Invariant: 0 <= portions_ <= capacity_, and capacity_ > 0.
class Stock
{
public:
    Stock(std::string name, int portions, int capacity)
        : name_(name), portions_(portions), capacity_(capacity)
    {
        if (capacity_ <= 0 || portions_ < 0 || portions_ > capacity_) {
            throw std::invalid_argument("Stock: bad starting values for " + name_);
        }
    }

    // Serves n portions if there are enough. Returns false and changes nothing otherwise.
    bool take(int n)
    {
        if (n < 0 || n > portions_) {
            return false;
        }
        portions_ = portions_ - n;
        return true;
    }

    // Adds n portions if they fit on the shelf. Returns false and changes nothing otherwise.
    bool add(int n)
    {
        if (n < 0 || n > capacity_ - portions_) {
            return false;
        }
        portions_ = portions_ + n;
        return true;
    }

    int portions() const { return portions_; }
    int capacity() const { return capacity_; }
    const std::string& name() const { return name_; }

    bool invariant_holds() const
    {
        return capacity_ > 0 && portions_ >= 0 && portions_ <= capacity_;
    }

private:
    std::string name_;
    int portions_;
    int capacity_;
};

void report(const Stock& s, const std::string& action, bool ok)
{
    std::cout << action << " -> " << (ok ? "done" : "refused") << "; " << s.name() << ": "
              << s.portions() << " of " << s.capacity()
              << ", invariant holds: " << s.invariant_holds() << '\n';
}

int main()
{
    Stock soup("soup", 10, 20);
    report(soup, "start", true);
    report(soup, "take(13)", soup.take(13));
    report(soup, "take(4)", soup.take(4));
    report(soup, "add(15)", soup.add(15));
    report(soup, "add(5)", soup.add(5));
    report(soup, "take(-2)", soup.take(-2));

    try {
        Stock bad("bread", 30, 20);
        report(bad, "start", true);
    } catch (const std::invalid_argument& e) {
        std::cout << "construction refused: " << e.what() << '\n';
    }
    return 0;
}
