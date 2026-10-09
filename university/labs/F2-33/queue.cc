// queue.cc: the kitchen's ticket printer. Tickets wait in a vector; the printer always
// takes the oldest one from the front. Bug report: "since we opened the second dining
// room the end-of-day run takes far longer than before".
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct Ticket
{
    int number;
    std::string text;
};

[[gnu::noinline]] Ticket take_oldest(std::vector<Ticket>& waiting)
{
    Ticket t = waiting.front();
    waiting.erase(waiting.begin());           // remove the first ticket
    return t;
}

[[gnu::noinline]] long print_all(std::vector<Ticket>& waiting)
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
    std::vector<Ticket> waiting;
    for (int i = 0; i < count; ++i) {
        waiting.push_back({i, "table " + std::to_string(i % 40) + ": soup"});
    }
    std::printf("printed %d tickets, %ld characters\n", count, print_all(waiting));
    return 0;
}
