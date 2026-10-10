#include <stdexcept>
#include <string>

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

private:
    std::string name_;
    int portions_;
    int capacity_;
};

int main()
{
    Stock soup("soup", 10, 20);
    soup.portions_ = -3;  // try to break the invariant from outside the class
    return 0;
}
