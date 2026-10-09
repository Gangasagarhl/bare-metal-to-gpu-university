// queue_fixed.cc: queue.cc after the forensic lab. A std::deque removes its first element
// without moving all the others.
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include <utility>

struct Ticket
{
    int number;
    std::string text;
};

[[gnu::noinline]] Ticket take_oldest(std::deque<Ticket>& waiting)
{
    Ticket t = std::move(waiting.front());
    waiting.pop_front();
    return t;
}

[[gnu::noinline]] long print_all(std::deque<Ticket>& waiting)
{
    long printed_chars = 0;
    while (!waiting.empty()) {
        const Ticket t = take_oldest(waiting);
        printed_chars += static_cast<long>(t.text.size());
    }
    return printed_chars;
}

int main(int argc, char** argv)
{
    const int count = argc > 1 ? std::atoi(argv[1]) : 10000;
    std::deque<Ticket> waiting;
    for (int i = 0; i < count; ++i) {
        waiting.push_back({i, "table " + std::to_string(i % 40) + ": soup"});
    }
    std::printf("printed %d tickets, %ld characters\n", count, print_all(waiting));
    return 0;
}
