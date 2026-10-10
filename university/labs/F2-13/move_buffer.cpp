#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Does a move copy the elements, or hand over the block of memory that holds them?
int main()
{
    std::vector<std::string> dishes(1000, "dish");
    const std::string* block = dishes.data();   // where the elements live now

    std::vector<std::string> copied = dishes;
    std::vector<std::string> moved = std::move(dishes);

    std::cout << "sizeof(std::vector<std::string>) = " << sizeof(moved) << '\n';
    std::cout << "copy uses the same block:  " << (copied.data() == block) << '\n';
    std::cout << "move uses the same block:  " << (moved.data() == block) << '\n';
    std::cout << "sizes: copied " << copied.size() << ", moved " << moved.size()
              << ", moved-from " << dishes.size() << " (this library)\n";
    return 0;
}
