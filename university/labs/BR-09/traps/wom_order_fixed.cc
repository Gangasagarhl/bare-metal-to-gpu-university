// wom_order_fixed.cc: the fix for wom_order.cc. Separate statements are sequenced:
// the first finishes before the second starts, on every conforming compiler.
#include <cstdio>
#include <string>

int next_id()
{
    static int n = 0;
    return ++n;
}

std::string range_label(int first, int second)
{
    return std::to_string(first) + "-" + std::to_string(second);
}

int main()
{
    const int first = next_id();
    const int second = next_id();
    const std::string label = range_label(first, second);
    const bool ok = label == "1-2";
    std::printf("label %s: test %s\n", label.c_str(), ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
