#include <string>

template <typename T>
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
    return larger(x, y).table;   // Order has no operator<
}
