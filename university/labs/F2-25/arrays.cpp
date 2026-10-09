#include <array>
#include <cstddef>
#include <iostream>
#include <span>
#include <vector>

// One function that accepts any contiguous run of ints and knows its length.
int total(std::span<const int> portions)
{
    int sum = 0;
    for (const int p : portions) {
        sum += p;
    }
    return sum;
}

int main()
{
    int cArray[4] = {1, 2, 3, 4};                  // built-in array: size is part of the type
    std::array<int, 3> fixed = {10, 20, 30};       // same layout, but a real value type
    std::vector<int> growing = {5, 5, 5, 5, 5};    // size chosen at run time, on the heap

    std::cout << "sizeof(cArray) = " << sizeof(cArray) << ", elements = " << std::size(cArray) << '\n';
    std::cout << "total(cArray)  = " << total(cArray) << '\n';
    std::cout << "total(fixed)   = " << total(fixed) << '\n';
    std::cout << "total(growing) = " << total(growing) << '\n';

    const std::span<const int> all(growing);
    const auto middle = all.subspan(1, 3);        // a view of elements 1, 2 and 3: no copy
    std::cout << "middle has " << middle.size() << " elements, total " << total(middle) << '\n';
    std::cout << "middle[0] is growing[1]? " << (&middle[0] == &growing[1] ? "yes" : "no") << '\n';
    std::cout << "sizeof(span) = " << sizeof(all) << " (a pointer and a length)\n";
    return 0;
}
