// F0-50 forensic evidence: a matrix multiply that passed the 1 x 1 test. Contains ONE deliberate mistake.
#include <cstddef>
#include <iostream>
#include <vector>

struct Matrix
{
    std::size_t rows;
    std::size_t cols;
    std::vector<double> data;  // row-major
    double& at(std::size_t r, std::size_t c) { return data[r * cols + c]; }
    double at(std::size_t r, std::size_t c) const { return data[r * cols + c]; }
};

Matrix multiply(const Matrix& a, const Matrix& b)
{
    Matrix c{a.rows, b.cols, std::vector<double>(a.rows * b.cols, 0.0)};
    double sum = 0.0;
    for (std::size_t i = 0; i < a.rows; ++i) {
        for (std::size_t j = 0; j < b.cols; ++j) {
            for (std::size_t p = 0; p < a.cols; ++p) {
                sum += a.at(i, p) * b.at(p, j);
            }
            c.at(i, j) = sum;
        }
    }
    return c;
}

void print(const char* name, const Matrix& m)
{
    std::cout << name << ":\n";
    for (std::size_t r = 0; r < m.rows; ++r) {
        for (std::size_t c = 0; c < m.cols; ++c) {
            std::cout << "  " << m.at(r, c);
        }
        std::cout << "\n";
    }
}

int main()
{
    print("test 1x1: [3] * [4]", multiply({1, 1, {3}}, {1, 1, {4}}));
    const Matrix a{2, 3, {1, 2, 3, 4, 5, 6}};
    const Matrix b{3, 2, {7, 8, 9, 10, 11, 12}};
    print("A * B", multiply(a, b));
    const Matrix identity{2, 2, {1, 0, 0, 1}};
    print("I * I", multiply(identity, identity));
    return 0;
}
