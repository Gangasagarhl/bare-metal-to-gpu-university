// bootargs_test.cpp - MP1 starter: host unit test for bootargs.h, written before the
// kernels use it (curriculum 19.2: tests first for pure logic). run_lab.sh builds it with
// -fsanitize=address,undefined and runs it; any failed case makes the exit status 1.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "bootargs.h"

namespace {

struct Case {
    std::string line;
    std::string key;
    bool found;
    uint64_t value;
};

} // namespace

int main()
{
    const std::vector<Case> cases = {
        {"cpus=4", "cpus", true, 4},
        {"test=all cpus=16 rootwait=3000", "rootwait", true, 3000},
        {"  cpus=2  ", "cpus", true, 2},
        {"xcpus=4", "cpus", false, 0},                 // only whole words match
        {"cpus=", "cpus", false, 0},                   // no digits
        {"cpus=4x", "cpus", false, 0},                 // trailing garbage
        {"cpus=18446744073709551615", "cpus", true, UINT64_MAX},
        {"cpus=18446744073709551616", "cpus", false, 0},   // one more: overflow refused
        {"", "cpus", false, 0},
        {"rootwait=0 cpus=1", "cpus", true, 1},
    };
    int failed = 0;
    for (const Case& c : cases) {
        uint64_t v = 0;
        bool found = bootargs::get_u64(c.line.c_str(), c.key.c_str(), &v);
        bool ok = found == c.found && (!found || v == c.value);
        std::cout << (ok ? "ok   " : "FAIL ") << '"' << c.line << "\" key " << c.key << " -> "
                  << (found ? std::to_string(v) : std::string("not found")) << '\n';
        failed += ok ? 0 : 1;
    }
    std::cout << cases.size() - static_cast<std::size_t>(failed) << " of " << cases.size()
              << " cases passed\n";
    return failed == 0 ? 0 : 1;
}
