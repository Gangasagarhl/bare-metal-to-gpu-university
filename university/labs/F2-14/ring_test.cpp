#include <iostream>
#include <string>

#include "ring_buffer.h"

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

int main()
{
    RingBuffer<int, 4> rail;
    int out = 0;
    expect(rail.empty() && rail.size() == 0, "new buffer is empty");
    expect(!rail.pop(out), "pop from empty is refused");
    for (int i = 1; i <= 4; ++i) {
        expect(rail.push(i), "push 1..4 into capacity 4");
    }
    expect(rail.full(), "full after 4 pushes");
    expect(!rail.push(5), "push into a full buffer is refused");
    expect(rail.pop(out) && out == 1, "first in, first out: 1");
    expect(rail.pop(out) && out == 2, "then 2");
    expect(rail.push(5) && rail.push(6), "wrap around: push 5 and 6");
    std::string order;
    while (rail.pop(out)) {
        order += std::to_string(out);
    }
    expect(order == "3456", "after wrap-around the order is 3 4 5 6");

    RingBuffer<std::string, 2> names;
    names.push("Amara");
    names.push("Jonas");
    std::string who;
    expect(names.pop(who) && who == "Amara", "works for std::string too");
    expect(RingBuffer<double, 8>::capacity() == 8, "capacity is the template argument");
    std::cout << "RingBuffer tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
