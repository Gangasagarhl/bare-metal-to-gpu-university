// safe.cpp - F11-08, the safe patterns: bounds, lifetimes and ownership done
// right. Built and run by run_lab.sh under -fsanitize=address,undefined; it must
// run cleanly and its checks must pass.
#include <cstdio>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

namespace {
int failures = 0;
void check(const char* what, bool ok) { std::printf("%s: %s\n", ok ? "ok" : "FAIL", what); failures += !ok; }

// Bounds: a function that reads element i, but refuses an out-of-range i. Taking
// a std::span means the length travels with the pointer -- no separate "and its
// size is N, trust me" argument to get wrong.
bool read_at(std::span<const int> data, std::size_t i, int& out)
{
    if (i >= data.size()) return false;        // the bound is checked, once, here
    out = data[i];
    return true;
}
}  // namespace

int main()
{
    // Bounds.
    std::vector<int> v{10, 20, 30};
    int got = 0;
    check("in-range span read", read_at(v, 2, got) && got == 30);
    check("out-of-range span read refused", !read_at(v, 5, got));
    bool threw = false;
    try { (void)v.at(9); } catch (const std::out_of_range&) { threw = true; }
    check(".at() throws on out-of-range", threw);

    // Lifetime: do not hold a pointer across a reallocation. Re-index instead.
    v.reserve(2000);                           // or copy the value out; here we reserve
    int first = v[0];
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    check("value copied out, not a stale pointer", first == 10);

    // Ownership: one owner (unique_ptr). Transfer with std::move; no double free.
    auto a = std::make_unique<int>(42);
    auto b = std::move(a);                     // ownership moves to b; a is now null
    check("ownership moved", a == nullptr && b != nullptr && *b == 42);

    std::printf(failures ? "SOME TESTS FAILED\n" : "ALL TESTS PASSED\n");
    return failures;
}
