#include <iostream>
#include <string>
#include <vector>

// Lab reference solution: choosing value, reference or const reference for each parameter.
struct Dish
{
    std::string name;
    int portions;
};

void swap_tables(int& a, int& b)              // must change both: references
{
    int old_a = a;
    a = b;
    b = old_a;
}

void restock(std::vector<Dish>& dishes, int extra)   // changes the caller's vector
{
    for (Dish& d : dishes) {
        d.portions = d.portions + extra;
    }
}

int total_portions(const std::vector<Dish>& dishes)  // only reads: const reference
{
    int total = 0;
    for (const Dish& d : dishes) {
        total = total + d.portions;
    }
    return total;
}

int doubled(int n)                             // small and only read: by value is fine
{
    return 2 * n;
}

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
    int x = 3;
    int y = 8;
    swap_tables(x, y);
    expect(x == 8 && y == 3, "swap_tables swaps");

    std::vector<Dish> dishes = {{"soup", 4}, {"rice", 0}, {"tea", 10}};
    expect(total_portions(dishes) == 14, "total before restock");
    restock(dishes, 5);
    expect(dishes[1].portions == 5, "restock changed the caller's dishes");
    expect(total_portions(dishes) == 29, "total after restock");
    expect(doubled(x) == 16, "doubled by value");
    std::cout << "reference tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
