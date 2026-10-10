#include <iostream>
#include <string>

struct Order
{
    int table = 0;
    std::string dish;
    int quantity = 1;
};

int main()
{
    Order soup{.dish = "soup", .table = 2};   // names given out of declaration order
    std::cout << soup.table << '\n';
    return 0;
}
