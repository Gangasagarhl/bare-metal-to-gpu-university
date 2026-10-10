// menu_fixed.cpp: menu.cc after the forensic lab. The special is copied (or looked up
// again after the vector stops growing) instead of kept as a reference into the vector.
#include <cstdio>
#include <string>
#include <vector>

struct Dish
{
    std::string name;
    int price_cents;
};

void print_menu(int extra)
{
    std::vector<Dish> menu;
    menu.push_back({"lentil soup with fresh bread", 650});
    const Dish special = menu.front();        // a copy: survives any reallocation
    for (int i = 0; i < extra; ++i) {
        menu.push_back({"dish of the day number " + std::to_string(i), 900 + i});
    }
    std::printf("dishes: %zu\n", menu.size());
    std::printf("special: %s, %d cents\n", special.name.c_str(), special.price_cents);
}

int main()
{
    print_menu(0);                            // a quiet day
    print_menu(8);                            // a busy day
    return 0;
}
