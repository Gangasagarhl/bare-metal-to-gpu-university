#include <iostream>
#include <string>

// Prints a line when an object starts and when it ends, so we can watch lifetimes.
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
    std::cout << "main starts\n";
    Tracer a("a");
    {
        std::cout << "inner block starts\n";
        Tracer b("b");
        Tracer c("c");
        std::cout << "inner block ends\n";
    }
    std::cout << "a station:\n";
    {
        Station s;
        std::cout << "  (using the station)\n";
    }
    std::cout << "main ends\n";
    return 0;
}
