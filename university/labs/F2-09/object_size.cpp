#include <iostream>

// Two ints in a struct with no member functions.
struct PlainPair
{
    int portions;
    int capacity;
};

// The same two ints, private, with member functions.
class CheckedPair
{
public:
    CheckedPair(int portions, int capacity) : portions_(portions), capacity_(capacity) {}
    bool take(int n)
    {
        if (n < 0 || n > portions_) {
            return false;
        }
        portions_ = portions_ - n;
        return true;
    }
    int portions() const { return portions_; }

private:
    int portions_;
    int capacity_;
};

int main()
{
    CheckedPair a(10, 20);
    CheckedPair b(3, 5);
    a.take(4);
    b.take(1);
    std::cout << "sizeof(int)         = " << sizeof(int) << '\n';
    std::cout << "sizeof(PlainPair)   = " << sizeof(PlainPair) << '\n';
    std::cout << "sizeof(CheckedPair) = " << sizeof(CheckedPair) << '\n';
    std::cout << "a.portions() = " << a.portions() << ", b.portions() = " << b.portions() << '\n';
    return 0;
}
