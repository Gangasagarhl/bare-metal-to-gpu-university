#include <iostream>
#include <string>

// By value: the function gets its own copy.
void add_salt_to_copy(std::string soup)
{
    soup += " + salt";
    std::cout << "  inside add_salt_to_copy: " << soup << '\n';
}

// By reference: the function works on the caller's own variable.
void add_salt(std::string& soup)
{
    soup += " + salt";
}

// By const reference: no copy, and the function promises not to change it.
void taste(const std::string& soup)
{
    std::cout << "  tasting: " << soup << " (" << soup.size() << " characters)\n";
}

// Two reference parameters: a classic use is swapping.
void swap_plates(int& left, int& right)
{
    int held = left;
    left = right;
    right = held;
}

int main()
{
    std::string pot = "tomato soup";
    add_salt_to_copy(pot);
    std::cout << "after add_salt_to_copy: " << pot << '\n';
    add_salt(pot);
    std::cout << "after add_salt:         " << pot << '\n';
    taste(pot);

    int a = 1;
    int b = 2;
    swap_plates(a, b);
    std::cout << "after swap_plates: a = " << a << ", b = " << b << '\n';
    return 0;
}
