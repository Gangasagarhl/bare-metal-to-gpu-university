#include <iostream>
#include <string>

int main()
{
    std::string name;
    int age = 0;
    std::cin >> name >> age;

    std::string greeting = "Hello, " + name + "!";
    std::cout << greeting << '\n';
    std::cout << "Next year you will be " << age + 1 << ".\n";
    std::cout << "Your name has " << name.size() << " letters.\n";
    return 0;
}
