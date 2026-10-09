#include <string>
#include <utility>

int main()
{
    std::string dish = "soup";
    dish = std::move(dish);   // moving an object into itself
    return static_cast<int>(dish.size());
}
