// Check De Morgan's laws by trying every combination of A and B.
#include <iostream>

int main()
{
    int rowsChecked = 0;
    int rowsWhereLaw1Fails = 0;
    int rowsWhereLaw2Fails = 0;
    std::cout << "A B | !(A&&B) !A||!B | !(A||B) !A&&!B\n";
    for (bool a : {false, true}) {
        for (bool b : {false, true}) {
            const bool left1 = !(a && b);
            const bool right1 = !a || !b;
            const bool left2 = !(a || b);
            const bool right2 = !a && !b;
            std::cout << a << ' ' << b << " |    " << left1 << "       " << right1
                      << "    |    " << left2 << "       " << right2 << '\n';
            ++rowsChecked;
            if (left1 != right1) {
                ++rowsWhereLaw1Fails;
            }
            if (left2 != right2) {
                ++rowsWhereLaw2Fails;
            }
        }
    }
    std::cout << "rows checked: " << rowsChecked << '\n';
    std::cout << "law 1 failed in " << rowsWhereLaw1Fails << " rows\n";
    std::cout << "law 2 failed in " << rowsWhereLaw2Fails << " rows\n";
    return 0;
}
