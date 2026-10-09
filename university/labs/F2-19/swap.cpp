#include <iostream>

void swapCopies(int a, int b)        // receives copies: the caller sees no change
{
    const int t = a;
    a = b;
    b = t;
}

void swapThrough(int* a, int* b)     // receives addresses: changes the caller's boxes
{
    const int t = *a;
    *a = *b;
    *b = t;
}

int main()
{
    int left = 1;
    int right = 2;
    swapCopies(left, right);
    std::cout << "after swapCopies:  left = " << left << ", right = " << right << '\n';
    swapThrough(&left, &right);
    std::cout << "after swapThrough: left = " << left << ", right = " << right << '\n';
    return 0;
}
