#include <iostream>
#include <stdexcept>
#include <string>

class Tracer
{
public:
    explicit Tracer(std::string name) : name_(name) { std::cout << "  acquire " << name_ << '\n'; }
    ~Tracer() { std::cout << "  release " << name_ << '\n'; }
    Tracer(const Tracer&) = delete;
    Tracer& operator=(const Tracer&) = delete;

private:
    std::string name_;
};

void fry(int portions)
{
    Tracer pan("pan");
    if (portions > 4) {
        throw std::length_error("the pan holds at most 4 portions");
    }
    std::cout << "  frying " << portions << '\n';
}

void cook_order(int portions)
{
    Tracer stove("stove");
    fry(portions);
    std::cout << "  order done\n";
}

int main()
{
    for (int portions : {2, 6}) {
        std::cout << "order of " << portions << ":\n";
        try {
            cook_order(portions);
        } catch (const std::length_error& e) {
            std::cout << "  caught in main: " << e.what() << '\n';
        }
    }
    return 0;
}
