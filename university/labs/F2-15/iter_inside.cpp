#include <iostream>
#include <list>
#include <vector>

// What an iterator is made of: compare its size with a pointer, and its address with data().
int main()
{
    std::vector<int> minutes = {12, 5, 20, 8};
    std::list<int> tables = {4, 9, 2};

    std::cout << "sizeof(int*)                       = " << sizeof(int*) << '\n';
    std::cout << "sizeof(std::vector<int>::iterator) = " << sizeof(std::vector<int>::iterator)
              << '\n';
    std::cout << "sizeof(std::list<int>::iterator)   = " << sizeof(std::list<int>::iterator)
              << '\n';

    auto it = minutes.begin() + 2;
    std::cout << "&*(begin + 2) == data() + 2: " << (&*it == minutes.data() + 2) << '\n';
    std::cout << "bytes from begin to end: "
              << (reinterpret_cast<const char*>(minutes.data() + minutes.size())
                  - reinterpret_cast<const char*>(minutes.data()))
              << '\n';

    auto l1 = tables.begin();
    auto l2 = std::next(l1);
    std::cout << "list neighbours 8 bytes apart in memory? "
              << ((reinterpret_cast<const char*>(&*l2) - reinterpret_cast<const char*>(&*l1))
                  == 8)
              << '\n';
    return 0;
}
