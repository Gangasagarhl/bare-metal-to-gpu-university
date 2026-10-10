#include <iostream>

// A reference has no size of its own you can ask for (sizeof gives the size of the object it
// names), but a class that stores a reference shows how much room the compiler uses for it.
struct HoldsInt
{
    int value;
};

struct HoldsRef
{
    int& value;
};

struct HoldsPointer
{
    int* value;
};

int main()
{
    int soup = 12;
    int& r = soup;
    HoldsRef h{soup};
    h.value = 13;
    std::cout << "sizeof(r) = " << sizeof(r) << " (the size of the int it names)\n";
    std::cout << "sizeof(HoldsInt) = " << sizeof(HoldsInt) << '\n';
    std::cout << "sizeof(HoldsRef) = " << sizeof(HoldsRef) << '\n';
    std::cout << "sizeof(HoldsPointer) = " << sizeof(HoldsPointer) << '\n';
    std::cout << "soup = " << soup << " (changed through the stored reference)\n";
    return 0;
}
