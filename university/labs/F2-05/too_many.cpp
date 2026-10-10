#include <array>
#include <iostream>

int main()
{
    std::array<int, 3> egg_box{1, 2, 3, 4};   // four eggs for a box of three
    std::cout << egg_box[0] << '\n';
    return 0;
}
