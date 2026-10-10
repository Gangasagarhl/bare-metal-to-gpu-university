#include <functional>
#include <iostream>
#include <string>
#include <vector>

// The kitchen bell: other parts of the program register what should happen when it rings.
class Bell
{
public:
    void on_ring(std::function<void(int)> action) { actions_.push_back(std::move(action)); }

    void ring(int table) const
    {
        for (const auto& action : actions_) {
            action(table);
        }
    }

private:
    std::vector<std::function<void(int)>> actions_;
};

int main()
{
    Bell bell;
    int rings = 0;
    std::string waiter = "Ana";
    bell.on_ring([&rings](int) { rings = rings + 1; });
    bell.on_ring([waiter](int table) { std::cout << waiter << " carries food to table " << table
                                                 << '\n'; });
    bell.ring(4);
    bell.ring(9);
    std::cout << "bell rang " << rings << " times\n";
    return 0;
}
