#include <iostream>
#include <vector>

int main()
{
    std::vector<int> eggs_per_box = {6, 6, 12};
    std::cout << "Box 0 has " << eggs_per_box.at(0) << " eggs" << std::endl;
    std::cout << "Box 3 has " << eggs_per_box.at(3) << " eggs" << std::endl;
    return 0;
}
