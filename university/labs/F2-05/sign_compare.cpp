#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> menu{"soup", "noodles", "rice"};
    for (int i = 0; i < menu.size(); ++i) {      // int compared with an unsigned size
        std::cout << menu[i] << '\n';
    }
    return 0;
}
