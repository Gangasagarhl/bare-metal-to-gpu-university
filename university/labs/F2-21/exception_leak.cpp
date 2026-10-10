#include <iostream>
#include <memory>
#include <stdexcept>

struct Pan
{
    Pan() { std::cout << "  pan taken\n"; }
    ~Pan() { std::cout << "  pan returned\n"; }
};

void cookRaw(bool burn)
{
    Pan* pan = new Pan;
    if (burn) {
        throw std::runtime_error("burnt");   // the delete below is skipped
    }
    delete pan;
}

void cookOwned(bool burn)
{
    auto pan = std::make_unique<Pan>();
    if (burn) {
        throw std::runtime_error("burnt");   // unique_ptr's destructor still runs
    }
}

int main()
{
    std::cout << std::unitbuf;   // print each line at once: the leak report ends the program
    for (const bool owned : {false, true}) {
        std::cout << (owned ? "cookOwned" : "cookRaw") << ":\n";
        try {
            if (owned) {
                cookOwned(true);
            } else {
                cookRaw(true);
            }
        } catch (const std::exception& e) {
            std::cout << "  caught: " << e.what() << '\n';
        }
    }
    return 0;
}
