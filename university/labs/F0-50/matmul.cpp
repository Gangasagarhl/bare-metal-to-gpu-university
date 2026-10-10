// F0-50 Listing 1: matrix multiplication C = A * B, checked against results computed by hand.
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
    for (std::size_t i = 0; i < a.rows; ++i) {
        for (std::size_t j = 0; j < b.cols; ++j) {
            double sum = 0.0;
            for (std::size_t p = 0; p < a.cols; ++p) {
                sum += a.at(i, p) * b.at(p, j);  // row i of A . column j of B
            }
            c.at(i, j) = sum;
        }
    }
    return c;
}

void print(const char* name, const Matrix& m)
{
    std::cout << name << " (" << m.rows << " x " << m.cols << "):\n";
    for (std::size_t r = 0; r < m.rows; ++r) {
        for (std::size_t c = 0; c < m.cols; ++c) {
            std::cout << "  " << m.at(r, c);
        }
        std::cout << "\n";
    }
}

bool check(const char* name, const Matrix& got, const Matrix& byHand)
{
    const bool ok = got.rows == byHand.rows && got.cols == byHand.cols && got.data == byHand.data;
    std::cout << name << ": " << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

int main()
{
    // Recipe (ingredients x dishes) times plan (dishes x days) = ingredients per day.
    const Matrix recipe{3, 2, {100, 200, 1, 3, 150, 50}};
    const Matrix plan{2, 2, {3, 1, 2, 1}};  // columns: Saturday, Sunday
    const Matrix perDay = multiply(recipe, plan);
    print("recipe * plan", perDay);

    const Matrix a{2, 3, {1, 2, 3, 4, 5, 6}};
    const Matrix b{3, 2, {7, 8, 9, 10, 11, 12}};
    print("A * B", multiply(a, b));
    print("B * A", multiply(b, a));

    const Matrix p{2, 2, {1, 2, 3, 4}};
    const Matrix swap{2, 2, {0, 1, 1, 0}};
    print("P * S", multiply(p, swap));
    print("S * P", multiply(swap, p));

    bool allOk = true;
    allOk = check("recipe * plan", perDay, {3, 2, {700, 300, 9, 4, 550, 200}}) && allOk;
    allOk = check("A * B", multiply(a, b), {2, 2, {58, 64, 139, 154}}) && allOk;
    allOk = check("P * S", multiply(p, swap), {2, 2, {2, 1, 4, 3}}) && allOk;
    allOk = check("S * P", multiply(swap, p), {2, 2, {3, 4, 1, 2}}) && allOk;
    std::cout << (allOk ? "all hand results match\n" : "MISMATCH\n");
    return allOk ? 0 : 1;
}
