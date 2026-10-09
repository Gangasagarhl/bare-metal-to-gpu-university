#include <memory>

// Does not compile: a unique_ptr cannot be copied, only moved.
int main()
{
    auto a = std::make_unique<int>(7);
    std::unique_ptr<int> b = a;
    return *b;
}
