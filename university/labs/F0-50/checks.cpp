// F0-50 number check: recomputes every number used in the chapter text.
#include <cstdint>
#include <iostream>

int main()
{
    std::cout << "recipe*plan Sat: " << 100 * 3 + 200 * 2 << " " << 1 * 3 + 3 * 2 << " " << 150 * 3 + 50 * 2
              << "; Sun: " << 100 * 1 + 200 * 1 << " " << 1 * 1 + 3 * 1 << " " << 150 * 1 + 50 * 1 << "\n";
    std::cout << "A*B: c00 = 1*7+2*9+3*11 = " << 1 * 7 + 2 * 9 + 3 * 11 << ", c01 = 1*8+2*10+3*12 = "
              << 1 * 8 + 2 * 10 + 3 * 12 << ", c10 = 4*7+5*9+6*11 = " << 4 * 7 + 5 * 9 + 6 * 11
              << ", c11 = 4*8+5*10+6*12 = " << 4 * 8 + 5 * 10 + 6 * 12 << "\n";
    // (AB)x = A(Bx) with A = [[1,2],[3,4]], B = [[0,1],[1,0]], x = (5,6)
    const int bx0 = 0 * 5 + 1 * 6, bx1 = 1 * 5 + 0 * 6;
    std::cout << "Bx = (" << bx0 << "," << bx1 << "), A(Bx) = (" << 1 * bx0 + 2 * bx1 << "," << 3 * bx0 + 4 * bx1
              << "), (AB)x with AB=[[2,1],[4,3]] = (" << 2 * 5 + 1 * 6 << "," << 4 * 5 + 3 * 6 << ")\n";
    // operation counts
    const std::uint64_t m = 3, k = 2, n = 2;
    std::cout << "recipe*plan multiply-adds m*n*k = " << m * n * k << "\n";
    std::cout << "2x3 times 3x2: " << 2 * 2 * 3 << " multiply-adds; 3x2 times 2x3: " << 3 * 3 * 2 << "\n";
    const std::uint64_t big = 4096;
    std::cout << "n = 4096: 2n^3 = " << 2 * big * big * big << " flops; 3n^2 = " << 3 * big * big
              << " numbers; bytes in FP32 = " << 3 * big * big * 4 << "\n";
    std::cout << "n = 1000: n^3 = " << 1000ULL * 1000 * 1000 << "; doubling n multiplies n^3 by " << 8 << "\n";
    // check yourself
    std::cout << "[[2,0],[1,3]]*[[1,4],[2,1]] = [[" << 2 * 1 + 0 * 2 << "," << 2 * 4 + 0 * 1 << "],["
              << 1 * 1 + 3 * 2 << "," << 1 * 4 + 3 * 1 << "]]\n";
    std::cout << "[[1,4],[2,1]]*[[2,0],[1,3]] = [[" << 1 * 2 + 4 * 1 << "," << 1 * 0 + 4 * 3 << "],["
              << 2 * 2 + 1 * 1 << "," << 2 * 0 + 1 * 3 << "]]\n";
    std::cout << "(1 2 3) * (4 5 6)^T = " << 1 * 4 + 2 * 5 + 3 * 6 << "; column times row is 3x3, first row: "
              << 1 * 4 << " " << 1 * 5 << " " << 1 * 6 << "\n";
    std::cout << "bug: running sums for A*B: " << 58 << " " << 58 + 64 << " " << 58 + 64 + 139 << " "
              << 58 + 64 + 139 + 154 << "; I*I bug: 1 1 1 2\n";
    std::cout << "100x50 times 50x20: shape 100x20, multiply-adds " << 100 * 20 * 50 << "\n";
    return 0;
}
