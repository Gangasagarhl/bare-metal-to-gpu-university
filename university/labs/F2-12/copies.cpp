#include <iostream>
#include <string>
#include <vector>

// Counts how often a Menu is copied, so we can see what passing by value costs.
int menu_copies = 0;

class Menu
{
public:
    Menu() = default;
    Menu(const Menu& other) : dishes_(other.dishes_)
    {
        menu_copies = menu_copies + 1;
    }
    Menu& operator=(const Menu&) = default;

    void add(const std::string& dish) { dishes_.push_back(dish); }
    std::size_t size() const { return dishes_.size(); }

private:
    std::vector<std::string> dishes_;
};

std::size_t count_by_value(Menu m)
{
    return m.size();
}

std::size_t count_by_const_ref(const Menu& m)
{
    return m.size();
}

int main()
{
    Menu menu;
    for (int i = 0; i < 1000; ++i) {
        menu.add("dish number " + std::to_string(i));
    }
    for (int i = 0; i < 10; ++i) {
        count_by_value(menu);
    }
    std::cout << "10 calls by value:     " << menu_copies << " copies of 1000 dishes\n";
    menu_copies = 0;
    for (int i = 0; i < 10; ++i) {
        count_by_const_ref(menu);
    }
    std::cout << "10 calls by const ref: " << menu_copies << " copies\n";
    return 0;
}
