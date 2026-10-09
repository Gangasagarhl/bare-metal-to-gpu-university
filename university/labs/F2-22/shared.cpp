#include <iostream>
#include <memory>
#include <string>

struct Recipe
{
    std::string name;
    explicit Recipe(std::string n) : name(std::move(n)) {}
    ~Recipe() { std::cout << "  recipe " << name << " destroyed\n"; }
};

int main()
{
    auto first = std::make_shared<Recipe>("bread");
    std::cout << "owners after make_shared: " << first.use_count() << '\n';
    {
        std::shared_ptr<Recipe> second = first;          // copy: one more owner
        std::cout << "owners inside the block:  " << first.use_count() << '\n';
    }
    std::cout << "owners after the block:   " << first.use_count() << '\n';

    std::weak_ptr<Recipe> watcher = first;               // observes, does not own
    std::cout << "weak_ptr does not count:  " << first.use_count() << '\n';
    if (auto locked = watcher.lock()) {
        std::cout << "watcher can still see " << locked->name << '\n';
    }
    std::cout << "last owner lets go\n";
    first.reset();
    std::cout << "watcher expired? " << (watcher.expired() ? "yes" : "no") << '\n';
    std::cout << "sizeof(shared_ptr<Recipe>) = " << sizeof(first) << '\n';
    return 0;
}
