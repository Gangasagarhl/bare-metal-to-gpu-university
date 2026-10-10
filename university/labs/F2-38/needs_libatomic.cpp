// Listing 5 (F2-38): asking at run time whether a large atomic is lock-free.
// With the course's standard build command this does not link (see the output).
#include <atomic>
#include <iostream>

struct Pair
{
    long a;
    long b;
};

int main()
{
    std::atomic<Pair> p{Pair{1, 2}};
    std::cout << std::boolalpha << "atomic<Pair> is_lock_free() at run time: " << p.is_lock_free() << '\n';
    return 0;
}
