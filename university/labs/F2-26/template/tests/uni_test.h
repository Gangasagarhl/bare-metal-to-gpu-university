// uni_test.h: the university's tiny unit-test harness (explained in F2-30).
// A test is a function registered with UNI_TEST; CHECK records a failure and continues.
// The test program returns 0 only if every check passed, so CTest and CI can use it.
#pragma once

#include <cmath>
#include <cstdio>
#include <string_view>
#include <vector>

namespace uni_test {

struct Case
{
    const char* name;
    void (*fn)();
};

inline std::vector<Case>& registry()
{
    static std::vector<Case> cases;
    return cases;
}

inline int& failed_checks()
{
    static int count = 0;
    return count;
}

struct Register
{
    Register(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline void check(bool ok, const char* expr, const char* file, int line)
{
    if (!ok) {
        ++failed_checks();
        std::printf("  %s:%d: CHECK failed: %s\n", file, line, expr);
    }
}

inline bool near(double a, double b, double tol)
{
    return std::fabs(a - b) <= tol;
}

// Runs every registered test whose name contains the filter (argv[1], optional).
inline int run_all(int argc, char** argv)
{
    const std::string_view filter = argc > 1 ? argv[1] : "";
    int ran = 0;
    int failed_tests = 0;
    for (const Case& c : registry()) {
        if (std::string_view(c.name).find(filter) == std::string_view::npos) {
            continue;
        }
        const int before = failed_checks();
        std::printf("[ RUN  ] %s\n", c.name);
        c.fn();
        const bool ok = failed_checks() == before;
        std::printf("[ %s ] %s\n", ok ? " OK " : "FAIL", c.name);
        ++ran;
        if (!ok) {
            ++failed_tests;
        }
    }
    std::printf("%d test(s) run, %d failed\n", ran, failed_tests);
    return (ran > 0 && failed_tests == 0) ? 0 : 1;
}

}  // namespace uni_test

#define UNI_TEST(name)                                         \
    static void name();                                        \
    static const uni_test::Register name##_registered(#name, name); \
    static void name()

#define CHECK(expr) uni_test::check(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) \
    uni_test::check(uni_test::near((a), (b), (tol)), #a " near " #b, __FILE__, __LINE__)
