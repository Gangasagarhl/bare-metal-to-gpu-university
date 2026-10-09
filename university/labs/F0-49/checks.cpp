// F0-49 number check: recomputes every number used in the chapter text.
#include <iostream>

int main()
{
    // recipe example
    std::cout << "flour 100*3 + 200*2 = " << 100 * 3 + 200 * 2 << ", eggs 1*3 + 3*2 = " << 1 * 3 + 3 * 2
              << ", milk 150*3 + 50*2 = " << 150 * 3 + 50 * 2 << "\n";
    std::cout << "column view: 3*(100,1,150) = (" << 3 * 100 << ", " << 3 * 1 << ", " << 3 * 150
              << "), 2*(200,3,50) = (" << 2 * 200 << ", " << 2 * 3 << ", " << 2 * 50 << ")\n";
    // worked example: M = [[2,1],[0,1]] acting on the unit square corners and on (3,2)
    const int m[2][2] = {{2, 1}, {0, 1}};
    const int pts[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (const auto& p : pts) {
        std::cout << "M(" << p[0] << "," << p[1] << ") = (" << m[0][0] * p[0] + m[0][1] * p[1] << ","
                  << m[1][0] * p[0] + m[1][1] * p[1] << ")  ";
    }
    std::cout << "\nM(3,2) = (" << 2 * 3 + 1 * 2 << "," << 0 * 3 + 1 * 2 << ")\n";
    // storage index formula for a 3 x 4 matrix
    std::cout << "3x4 row-major index of (2,1) = 2*4 + 1 = " << 2 * 4 + 1
              << "; column-major index = 1*3 + 2 = " << 1 * 3 + 2 << "\n";
    // matrix addition and scaling
    std::cout << "[[1,2],[3,4]] + [[5,6],[7,8]] = [[" << 1 + 5 << "," << 2 + 6 << "],[" << 3 + 7 << ","
              << 4 + 8 << "]], 3*[[1,2],[3,4]] = [[" << 3 << "," << 6 << "],[" << 9 << "," << 12 << "]]\n";
    // shop bills: correct and with the bug
    std::cout << "bill A = 3*2 + 2*1 + 4*3 = " << 3 * 2 + 2 * 1 + 4 * 3 << ", bill B = 2*2 + 3*1 + 5*3 = "
              << 2 * 2 + 3 * 1 + 5 * 3 << ", buggy B = 4*2 + 2*1 + 3*3 = " << 4 * 2 + 2 * 1 + 3 * 3 << "\n";
    std::cout << "buggy index r*rows + c for r=1: " << 1 * 2 + 0 << " " << 1 * 2 + 1 << " " << 1 * 2 + 2
              << "; correct r*cols + c: " << 1 * 3 + 0 << " " << 1 * 3 + 1 << " " << 1 * 3 + 2 << "\n";
    // check yourself
    std::cout << "[[1,2,0],[0,1,3]](2,1,1) = (" << 1 * 2 + 2 * 1 + 0 * 1 << "," << 0 * 2 + 1 * 1 + 3 * 1
              << ")\n";
    std::cout << "[[0,-1],[1,0]](1,0) = (0,1), (0,1) -> (" << 0 * 0 + -1 * 1 << "," << 1 * 0 + 0 * 1 << ")\n";
    std::cout << "4x5 matrix entries " << 4 * 5 << ", index of (3,2) = " << 3 * 5 + 2 << "\n";
    std::cout << "pancakes 4 cake 1: flour " << 100 * 4 + 200 * 1 << ", eggs " << 1 * 4 + 3 * 1 << ", milk "
              << 150 * 4 + 50 * 1 << "\n";
    std::cout << "wheels [[0.5,0.5],[-1,1]]: (2,2) -> (" << 0.5 * 2 + 0.5 * 2 << "," << -1 * 2 + 1 * 2
              << "), (1,3) -> (" << 0.5 * 1 + 0.5 * 3 << "," << -1 * 1 + 1 * 3 << ")\n";
    const int data[6] = {100, 200, 1, 3, 150, 50};
    std::cout << "column-major read of row-major data, rows=3:";
    for (int r = 0; r < 3; ++r) {
        std::cout << " /";
        for (int c = 0; c < 2; ++c) {
            std::cout << " " << data[c * 3 + r];
        }
    }
    std::cout << "\n";
    return 0;
}
