#include <iostream>
#include <string>

// Worked example: the same Tracer and Station as lifetime.cpp, a different main.
class Tracer
{
public:
    explicit Tracer(std::string name) : name_(name)
    {
        std::cout << "  construct " << name_ << '\n';
    }

    ~Tracer()
    {
        std::cout << "  destroy   " << name_ << '\n';
    }

private:
    std::string name_;
};

// A class with two Tracer members: they are built in the order they are declared.
class Station
{
public:
    Station() : board_("board"), stove_("stove")
    {
        std::cout << "  Station constructor body runs\n";
    }

    ~Station()
    {
        std::cout << "  Station destructor body runs\n";
    }

private:
    Tracer board_;  // declared first, so built first
    Tracer stove_;
};

int main()
{
    Tracer x("x");
    {
        Tracer y("y");
        Station s;
    }
    Tracer z("z");
    return 0;
}
