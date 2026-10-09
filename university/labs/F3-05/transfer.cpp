// transfer.cpp - user-space version of the lesson: take two locks with std::scoped_lock,
// which acquires several mutexes without deadlock whatever order the callers name them in.
#include <cstdio>
#include <mutex>
#include <thread>

struct Shelf {
    std::mutex m;
    int books = 100;
};

void move(Shelf& from, Shelf& to, int times)
{
    for (int i = 0; i < times; ++i) {
        std::scoped_lock both(from.m, to.m);  // locks both, avoiding deadlock
        --from.books;
        ++to.books;
    }
}

int main()
{
    Shelf left, right;
    std::thread a(move, std::ref(left), std::ref(right), 100000);  // left -> right
    std::thread b(move, std::ref(right), std::ref(left), 100000);  // right -> left: opposite order
    a.join();
    b.join();
    std::printf("left %d, right %d, total %d (both threads finished)\n", left.books, right.books,
                left.books + right.books);
    return 0;
}
