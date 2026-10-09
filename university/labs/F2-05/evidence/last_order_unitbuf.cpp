#include <cstddef>
#include <iostream>
#include <vector>

int main()
{
    std::cout << std::unitbuf;           // evidence only: send each output at once
    std::vector<int> tables_waiting{4, 7, 2, 9};
    int served = 0;
    for (std::size_t i = 0; i <= tables_waiting.size(); ++i) {
        std::cout << "Serving table " << tables_waiting[i] << '\n';
        ++served;
    }
    std::cout << "Served " << served << " tables.\n";
    return 0;
}
