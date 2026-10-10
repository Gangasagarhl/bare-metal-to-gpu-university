#include <functional>
#include <iostream>

// A lambda that captures a local variable by reference and outlives it.
std::function<int()> make_reader()
{
    int portions = 7;
    return [&portions]() { return portions; };   // portions dies when make_reader returns
}

int main()
{
    std::cout << std::unitbuf;
    std::function<int()> read_portions = make_reader();
    std::cout << "portions: " << read_portions() << '\n';
    return 0;
}
