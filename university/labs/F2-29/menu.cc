// menu.cc: builds today's menu and prints the first dish as the "special".
// Bug report: "the special is printed as garbage, but only on busy days with many dishes".
#include <cstdio>
#include <string>
#include <vector>

struct Dish
{
    std::string name;
    int price_cents;
};

int main(int argc, char**)
{
    std::vector<Dish> menu;
    menu.push_back({"lentil soup with fresh bread", 650});
    const Dish& special = menu.front();      // a reference to the first element

    const int extra = argc > 1 ? 8 : 0;       // a busy day: the cook adds more dishes
    for (int i = 0; i < extra; ++i) {
        menu.push_back({"dish of the day number " + std::to_string(i), 900 + i});
    }
    std::printf("dishes: %zu\n", menu.size());
    std::printf("special: %s, %d cents\n", special.name.c_str(), special.price_cents);
    return 0;
}
