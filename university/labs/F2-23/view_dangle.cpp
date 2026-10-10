#include <iostream>
#include <string>
#include <string_view>

std::string makeLabel(int table)
{
    return "table " + std::to_string(table) + ", window seat";
}

// A string_view does not own characters: here it outlives the string it looks at.
int main()
{
    std::cout << std::unitbuf;
    const std::string_view label = makeLabel(12);   // the temporary string dies at the ';'
    std::cout << "label: " << label << '\n';
    return 0;
}
