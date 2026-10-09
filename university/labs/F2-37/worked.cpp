// Worked-example check (F2-37): a ring buffer of capacity 4, as in BoundedQueue.
// Prints head, count and the slot each operation uses.
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::size_t capacity = 4;
    std::vector<std::string> slots(capacity, "-");
    std::size_t head = 0;
    std::size_t count = 0;
    const std::string ops = "PPPOPPPO";  // P = push the next letter, O = pop
    char next = 'a';
    for (char op : ops) {
        if (op == 'P') {
            if (count == capacity) {
                std::cout << "push " << next << ": FULL, a producer would wait\n";
                continue;
            }
            const std::size_t slot = (head + count) % capacity;
            slots[slot] = std::string(1, next);
            ++count;
            std::cout << "push " << next << " into slot " << slot;
            ++next;
        } else {
            const std::size_t slot = head;
            std::cout << "pop  " << slots[slot] << " from slot " << slot;
            slots[slot] = "-";
            head = (head + 1) % capacity;
            --count;
        }
        std::cout << "  -> head " << head << ", count " << count << ", slots [";
        for (std::size_t i = 0; i < capacity; ++i) {
            std::cout << slots[i] << (i + 1 < capacity ? " " : "]\n");
        }
    }
    return 0;
}
