#include <iostream>

int double_it(int number)
{
    int answer = number * 2;
    return answer;
}

int main()
{
    int cups = 3;
    int plates = double_it(cups);
    std::cout << "Plates: " << plates << '\n';
    return 0;
}
