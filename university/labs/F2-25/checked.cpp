#include <array>
#include <iostream>
#include <stdexcept>

// .at() checks the index and throws; operator[] does not check.
int main()
{
    std::array<int, 4> seats = {1, 2, 3, 4};
    try {
        std::cout << "seats.at(2) = " << seats.at(2) << '\n';
        std::cout << "seats.at(4) = " << seats.at(4) << '\n';
    } catch (const std::out_of_range& e) {
        std::cout << "caught std::out_of_range: " << e.what() << '\n';
    }
    return 0;
}
