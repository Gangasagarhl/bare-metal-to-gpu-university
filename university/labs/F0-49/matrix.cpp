// F0-49 Listing 1: a matrix as a table of numbers, stored row by row, times a vector.
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

class Matrix
{
public:
    Matrix(std::size_t rows, std::size_t cols, std::vector<double> values)
        : rows_(rows), cols_(cols), data_(std::move(values))
    {
    }
    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }
    double at(std::size_t r, std::size_t c) const { return data_[r * cols_ + c]; }
    const std::vector<double>& storage() const { return data_; }

private:
    std::size_t rows_;
    std::size_t cols_;
    std::vector<double> data_;  // row-major: row 0, then row 1, ...
};

// Row view: entry r of A*x is (row r of A) . x
std::vector<double> timesByRows(const Matrix& a, const std::vector<double>& x)
{
    std::vector<double> y(a.rows(), 0.0);
    for (std::size_t r = 0; r < a.rows(); ++r) {
        for (std::size_t c = 0; c < a.cols(); ++c) {
            y[r] += a.at(r, c) * x[c];
        }
    }
    return y;
}

// Column view: A*x = x[0] * (column 0) + x[1] * (column 1) + ...
std::vector<double> timesByColumns(const Matrix& a, const std::vector<double>& x)
{
    std::vector<double> y(a.rows(), 0.0);
    for (std::size_t c = 0; c < a.cols(); ++c) {
        for (std::size_t r = 0; r < a.rows(); ++r) {
            y[r] += x[c] * a.at(r, c);
        }
    }
    return y;
}

void printList(const char* label, const std::vector<double>& v)
{
    std::cout << label << ":";
    for (double e : v) {
        std::cout << " " << e;
    }
    std::cout << "\n";
}

int main()
{
    // rows: flour (g), eggs, milk (ml); columns: one batch of pancakes, one cake
    const Matrix recipe(3, 2, {100, 200,
                               1, 3,
                               150, 50});
    std::cout << "recipe matrix, " << recipe.rows() << " x " << recipe.cols() << ":\n";
    for (std::size_t r = 0; r < recipe.rows(); ++r) {
        for (std::size_t c = 0; c < recipe.cols(); ++c) {
            std::cout << std::setw(6) << recipe.at(r, c);
        }
        std::cout << "\n";
    }
    printList("storage order in memory", recipe.storage());
    std::cout << "entry (row 2, column 0) is storage index " << 2 * recipe.cols() + 0 << "\n";

    const std::vector<double> batches{3, 2};  // 3 batches of pancakes, 2 cakes
    printList("shopping list by rows", timesByRows(recipe, batches));
    printList("shopping list by columns", timesByColumns(recipe, batches));
    return 0;
}
