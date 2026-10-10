// Worked-example check (F2-42): SpscRing's counters only grow; the count of stored items is
// tail - head in unsigned arithmetic, which stays correct even when the counters wrap around.
#include <cstdint>
#include <iostream>

int main()
{
    const std::uint32_t capacity = 8;
    const std::uint32_t mask = capacity - 1;
    struct Case
    {
        std::uint32_t head;
        std::uint32_t tail;
    };
    const Case cases[] = {{0, 3}, {5, 13}, {4294967294u, 4294967295u}, {4294967294u, 2u}};
    for (const Case& c : cases) {
        const std::uint32_t count = c.tail - c.head;  // unsigned subtraction wraps modulo 2^32
        std::cout << "head " << c.head << ", tail " << c.tail << ": items stored " << count << ", next read slot "
                  << (c.head & mask) << ", next write slot " << (c.tail & mask) << (count == capacity ? " (FULL)" : "")
                  << '\n';
    }
    return 0;
}
