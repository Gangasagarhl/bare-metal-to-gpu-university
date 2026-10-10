// useafterfree.cc - F11-08, demo: ownership confusion. One part of the program
// owns an object through a unique_ptr; another keeps a raw "observer" pointer to
// it. When the owner resets (frees) the object, the observer is left dangling.
// The bug is not the raw pointer itself but using it after the owner is gone.
// Built with -fsanitize=address: ASan reports heap-use-after-free.
#include <cstdio>
#include <memory>

struct Sensor {
    int id;
    int last;
};

int main()
{
    auto owner = std::make_unique<Sensor>(Sensor{7, 100});
    Sensor* observer = owner.get();            // a non-owning view: fine, so far
    std::printf("observer: sensor %d last %d\n", observer->id, observer->last);

    owner.reset();                              // the owner frees the Sensor here
    // ... later, code that did not know about the reset uses the observer:
    std::printf("observer: sensor %d last %d\n", observer->id, observer->last);  // use-after-free
    return 0;
}
