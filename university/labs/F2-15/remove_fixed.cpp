#include <algorithm>
#include <iostream>
#include <vector>

void print(const char* label, const std::vector<int>& v)
{
    std::cout << label << " (size " << v.size() << "):";
    for (int t : v) {
        std::cout << ' ' << t;
    }
    std::cout << '\n';
}

int main()
{
    std::vector<int> tables = {4, 0, 9, 0, 2, 7};
    auto new_end = std::remove(tables.begin(), tables.end(), 0);   // moves keepers to the front
    std::cout << "keepers: " << (new_end - tables.begin()) << '\n';
    tables.erase(new_end, tables.end());                          // now really shrink
    print("erase-remove", tables);

    std::vector<int> again = {4, 0, 9, 0, 2, 7};
    auto removed = std::erase(again, 0);                           // C++20: both steps in one
    std::cout << "std::erase removed " << removed << '\n';
    print("std::erase  ", again);
    return 0;
}
