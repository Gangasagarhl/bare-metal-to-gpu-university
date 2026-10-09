#include <iostream>
#include <string>

// What the compiler builds for a lambda: an object of a new class type.
// Its captures are that object's data members; its body is that class's operator().
int main()
{
    int limit = 10;
    double price = 4.5;
    std::string dish = "soup";

    auto none = []() { return 1; };
    auto one_int = [limit]() { return limit; };
    auto int_and_double = [limit, price]() { return limit * price; };
    auto by_ref = [&limit]() { return limit; };
    auto a_string = [dish]() { return dish.size(); };

    std::cout << "sizeof(int) " << sizeof(int) << ", sizeof(double) " << sizeof(double)
              << ", sizeof(std::string) " << sizeof(std::string) << '\n';
    std::cout << "no capture:        " << sizeof(none) << '\n';
    std::cout << "[limit]:           " << sizeof(one_int) << '\n';
    std::cout << "[limit, price]:    " << sizeof(int_and_double) << '\n';
    std::cout << "[&limit]:          " << sizeof(by_ref) << '\n';
    std::cout << "[dish]:            " << sizeof(a_string) << '\n';
    std::cout << "results: " << none() << ' ' << one_int() << ' ' << int_and_double() << ' '
              << by_ref() << ' ' << a_string() << '\n';
    return 0;
}
