// F0-49 forensic evidence: two shops' bills from a price table. Contains ONE deliberate mistake.
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <vector>

struct Matrix
{
    std::size_t rows;
    std::size_t cols;
    std::vector<double> data;  // row-major
    double at(std::size_t r, std::size_t c) const { return data[r * rows + c]; }
};

int main()
{
    // rows: shop A, shop B; columns: price of bread, milk, eggs (coins per item)
    const Matrix prices{2, 3, {3, 2, 4,
                               2, 3, 5}};
    const std::vector<double> basket{2, 1, 3};  // 2 bread, 1 milk, 3 eggs
    std::cout << "input data, in file order: 3 2 4 2 3 5\n";
    std::cout << "price table as the program reads it:\n";
    for (std::size_t r = 0; r < prices.rows; ++r) {
        std::cout << (r == 0 ? "  shop A:" : "  shop B:");
        for (std::size_t c = 0; c < prices.cols; ++c) {
            std::cout << std::setw(4) << prices.at(r, c);
        }
        std::cout << "\n";
    }
    for (std::size_t r = 0; r < prices.rows; ++r) {
        double bill = 0.0;
        for (std::size_t c = 0; c < prices.cols; ++c) {
            bill += prices.at(r, c) * basket[c];
        }
        std::cout << (r == 0 ? "bill at shop A: " : "bill at shop B: ") << bill << "\n";
    }
    return 0;
}
