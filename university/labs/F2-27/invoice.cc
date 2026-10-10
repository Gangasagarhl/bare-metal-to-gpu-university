// invoice.cc: the shop's invoice tool. Bug report: "orders of 10 or more items never
// get the 10 % discount". The team's build uses -Wall -Wextra -Wshadow, without -Werror.
#include <cstdio>
#include <string>
#include <vector>

struct Line
{
    std::string item;
    int quantity;
    double unit_price;
};

double line_total(const Line& line, bool with_tax)
{
    return line.quantity * line.unit_price;
}

double discount_rate(int items)
{
    double rate = 0.0;
    if (items >= 10) {
        double rate = 0.10;
    }
    return rate;
}

int count_items(const std::vector<Line>& lines)
{
    int n = 0;
    for (int i = 0; i < lines.size(); ++i) {
        n += lines[i].quantity;
    }
    return n;
}

int main()
{
    const std::vector<Line> lines = {{"pencil", 8, 0.50}, {"eraser", 4, 0.25}};
    double subtotal = 0.0;
    for (const Line& line : lines) {
        subtotal += line_total(line, false);
    }
    const int items = count_items(lines);
    const double total = subtotal * (1.0 - discount_rate(items));
    std::printf("items %d  subtotal %.2f  total %.2f\n", items, subtotal, total);
    return 0;
}
