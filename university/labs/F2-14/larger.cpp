#include <iostream>
#include <string>

// One recipe with a blank: T is filled in by the compiler for each type it is used with.
template <typename T>
T larger(T a, T b)
{
    return (a < b) ? b : a;
}

int main()
{
    std::cout << larger(3, 8) << '\n';                                 // T = int
    std::cout << larger(2.5, 1.25) << '\n';                            // T = double
    std::cout << larger(std::string("rice"), std::string("soup")) << '\n';  // T = std::string
    std::cout << larger<double>(3, 2.5) << '\n';                       // T chosen by hand
    return 0;
}
