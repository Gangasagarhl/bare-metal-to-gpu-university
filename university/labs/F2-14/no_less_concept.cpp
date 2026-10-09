#include <concepts>
#include <string>

template <std::totally_ordered T>   // a concept: states what T must be able to do
T larger(T a, T b)
{
    return (a < b) ? b : a;
}

struct Order
{
    int table;
    std::string dish;
};

int main()
{
    Order x{3, "rice"};
    Order y{5, "soup"};
    return larger(x, y).table;
}
