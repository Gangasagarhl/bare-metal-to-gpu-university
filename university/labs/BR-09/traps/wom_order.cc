// wom_order.cc: "works on my machine" trap 1. A label maker and its test.
// The test passes with one compiler and fails with another, with no warning from either.
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
    // Bug: the two calls are arguments of one function call. C++ does not say which
    // argument is evaluated first, so either call may return 1.
    const std::string label = range_label(next_id(), next_id());
    const bool ok = label == "1-2";
    std::printf("label %s: test %s\n", label.c_str(), ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
