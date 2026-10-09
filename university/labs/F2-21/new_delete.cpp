#include <iostream>
#include <string>

struct Dish
{
    std::string name;
    explicit Dish(std::string n) : name(std::move(n)) { std::cout << "  made " << name << '\n'; }
    ~Dish() { std::cout << "  destroyed " << name << '\n'; }
};

// Raw new and delete, used correctly (this chapter only; later code uses smart pointers).
int main()
{
    std::cout << "new Dish\n";
    Dish* soup = new Dish("soup");      // allocate heap memory AND construct the object
    std::cout << "using " << soup->name << '\n';
    delete soup;                        // destroy the object AND free the memory
    soup = nullptr;                     // the old address is now meaningless

    std::cout << "new int[5]\n";
    int* counts = new int[5]{1, 2, 3, 4, 5};
    int sum = 0;
    for (int i = 0; i < 5; ++i) {
        sum += counts[i];
    }
    std::cout << "sum = " << sum << '\n';
    delete[] counts;                    // array form for memory from new[]
    return 0;
}
