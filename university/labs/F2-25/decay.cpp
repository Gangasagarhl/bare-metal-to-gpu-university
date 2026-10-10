#include <iostream>

// Does not compile with the course flags: inside the function the "array" is only a pointer.
int count(int portions[8])
{
    return sizeof(portions) / sizeof(portions[0]);
}

int main()
{
    int portions[8] = {};
    std::cout << count(portions) << '\n';
    return 0;
}
