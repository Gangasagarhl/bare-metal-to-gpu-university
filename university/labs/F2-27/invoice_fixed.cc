// invoice_fixed.cc: invoice.cc after reading every warning.
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

struct Line
{
    std::string item;
    int quantity;
    double unit_price;
};

double line_total(const Line& line)           // the unused parameter is removed
{
    return line.quantity * line.unit_price;
}

double discount_rate(int items)
{
    double rate = 0.0;
    if (items >= 10) {
        rate = 0.10;                          // the root cause: assign, do not declare
    }
    return rate;
}

int count_items(const std::vector<Line>& lines)
{
    int n = 0;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        n += lines[i].quantity;
    }
    return n;
}

int main()
{
    const std::vector<Line> lines = {{"pencil", 8, 0.50}, {"eraser", 4, 0.25}};
    double subtotal = 0.0;
    for (const Line& line : lines) {
        subtotal += line_total(line);
    }
    const int items = count_items(lines);
    const double total = subtotal * (1.0 - discount_rate(items));
    std::printf("items %d  subtotal %.2f  total %.2f\n", items, subtotal, total);
    return 0;
}
