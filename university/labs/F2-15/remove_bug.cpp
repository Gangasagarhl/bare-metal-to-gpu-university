#include <algorithm>
#include <iostream>
#include <vector>

// Forensic evidence: the kitchen screen should drop cancelled orders (table number 0).
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
    print("before", tables);
    std::remove(tables.begin(), tables.end(), 0);    // "remove the cancelled orders"
    print("after ", tables);
    return 0;
}
