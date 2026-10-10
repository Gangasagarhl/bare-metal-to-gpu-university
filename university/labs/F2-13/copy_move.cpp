#include <iostream>
#include <string>
#include <utility>
#include <vector>

// A tray of dishes that reports every copy and every move.
class Tray
{
public:
    explicit Tray(std::vector<std::string> dishes) : dishes_(std::move(dishes)) {}

    Tray(const Tray& other) : dishes_(other.dishes_)
    {
        std::cout << "  copy constructor\n";
    }

    Tray(Tray&& other) noexcept : dishes_(std::move(other.dishes_))
    {
        std::cout << "  move constructor\n";
    }

    Tray& operator=(const Tray& other)
    {
        dishes_ = other.dishes_;
        std::cout << "  copy assignment\n";
        return *this;
    }

    Tray& operator=(Tray&& other) noexcept
    {
        dishes_ = std::move(other.dishes_);
        std::cout << "  move assignment\n";
        return *this;
    }

    ~Tray() = default;

    std::size_t size() const { return dishes_.size(); }

private:
    std::vector<std::string> dishes_;
};

Tray make_tray()
{
    return Tray({"soup", "bread", "tea"});
}

int main()
{
    std::cout << "1. Tray a(...)\n";
    Tray a({"rice", "beans"});
    std::cout << "2. Tray b = a;\n";
    Tray b = a;
    std::cout << "3. Tray c = std::move(a);\n";
    Tray c = std::move(a);
    std::cout << "4. b = c;\n";
    b = c;
    std::cout << "5. b = make_tray();\n";
    b = make_tray();
    std::cout << "6. Tray d = make_tray();\n";
    Tray d = make_tray();
    std::cout << "sizes: a=" << a.size() << " b=" << b.size() << " c=" << c.size()
              << " d=" << d.size() << '\n';
    return 0;
}
