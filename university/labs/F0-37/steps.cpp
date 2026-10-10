#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> steps = {"wash the rice", "add water", "cook until soft"};
    for (std::size_t i = 0; i <= steps.size(); ++i) {
        std::cout << "Step " << i + 1 << ": " << steps.at(i) << std::endl;
    }
    std::cout << "The rice is ready." << std::endl;
    return 0;
}
