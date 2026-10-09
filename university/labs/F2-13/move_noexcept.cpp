#include <iostream>
#include <vector>

int copies = 0;
int moves = 0;

struct SafeMove                       // move constructor promises not to throw
{
    SafeMove() = default;
    SafeMove(const SafeMove&) { copies = copies + 1; }
    SafeMove(SafeMove&&) noexcept { moves = moves + 1; }
};

struct RiskyMove                      // same, but without noexcept
{
    RiskyMove() = default;
    RiskyMove(const RiskyMove&) { copies = copies + 1; }
    RiskyMove(RiskyMove&&) { moves = moves + 1; }
};

template <typename T>
void grow(const char* name)
{
    copies = 0;
    moves = 0;
    std::vector<T> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(T());
    }
    std::cout << name << ": " << copies << " copies, " << moves << " moves\n";
}

int main()
{
    grow<SafeMove>("noexcept move   ");
    grow<RiskyMove>("move may throw  ");
    return 0;
}
