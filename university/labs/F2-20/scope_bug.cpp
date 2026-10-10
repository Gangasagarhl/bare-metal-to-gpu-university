#include <iostream>

// A pointer that outlives the block whose local it points to.
int main()
{
    std::cout << std::unitbuf;
    int* bill = nullptr;
    {
        int total = 42;          // lives only until the closing brace
        bill = &total;
        std::cout << "inside the block: *bill = " << *bill << '\n';
    }
    std::cout << "after the block: *bill = " << *bill << '\n';   // the box was handed back
    return 0;
}
