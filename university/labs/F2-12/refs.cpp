#include <iostream>

// Three ways to hand a number to a function.
void discount_copy(int price)        // gets its own copy
{
    price = price - 2;
}

void discount_ref(int& price)        // gets another name for the caller's variable
{
    price = price - 2;
}

int with_tax(const int& price)       // may look, may not change
{
    return price + price / 10;
}

int main()
{
    int soup = 12;
    int& also_soup = soup;           // a reference: a second name for the same int
    also_soup = 15;
    std::cout << "soup = " << soup << ", same object: " << (&also_soup == &soup) << '\n';

    discount_copy(soup);
    std::cout << "after discount_copy: " << soup << '\n';
    discount_ref(soup);
    std::cout << "after discount_ref:  " << soup << '\n';
    std::cout << "with_tax(soup) = " << with_tax(soup) << ", soup still " << soup << '\n';
    return 0;
}
