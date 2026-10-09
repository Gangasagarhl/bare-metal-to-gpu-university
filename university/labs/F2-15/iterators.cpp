#include <iostream>
#include <iterator>
#include <list>
#include <vector>

// The same function works for any container that offers begin() and end().
template <typename Container>
void print_all(const Container& c)
{
    for (auto it = c.begin(); it != c.end(); ++it) {   // [begin, end): end is one past the last
        std::cout << *it << ' ';
    }
    std::cout << '\n';
}

int main()
{
    std::vector<int> minutes = {12, 5, 20, 8};
    std::list<int> tables = {4, 9, 2};
    print_all(minutes);
    print_all(tables);

    auto first = minutes.begin();
    auto last = minutes.end();
    std::cout << "elements between begin and end: " << std::distance(first, last) << '\n';
    std::cout << "*begin = " << *first << ", *(end - 1) = " << *(last - 1) << '\n';

    std::cout << "vector iterator is contiguous: "
              << std::contiguous_iterator<std::vector<int>::iterator> << '\n';
    std::cout << "list iterator is contiguous:   "
              << std::contiguous_iterator<std::list<int>::iterator> << '\n';
    std::cout << "list iterator is random access: "
              << std::random_access_iterator<std::list<int>::iterator> << '\n';
    return 0;
}
