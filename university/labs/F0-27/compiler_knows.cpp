// This file is EXPECTED not to compile with the university's flags (-Werror).
// It compares !(a || b) with !a && !b directly, in one expression.
#include <iostream>

int main()
{
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            std::cout << (!(a || b) == (!a && !b)) << '\n';
        }
    }
    return 0;
}
