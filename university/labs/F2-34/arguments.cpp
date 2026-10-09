// Listing 2 (F2-34): how arguments reach a thread: copied by default, std::ref to share, move for owners.
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

void addOne(int value)  // receives a copy
{
    value += 1;
}

void addOneShared(int& value)  // receives a reference to the caller's object
{
    value += 1;
}

void takeOwnership(std::unique_ptr<std::string> order)
{
    std::cout << "worker now owns: " << *order << '\n';
}

int main()
{
    int counter = 10;

    std::thread a(addOne, counter);  // the thread gets its own copy of 10
    a.join();
    std::cout << "after addOne:       counter = " << counter << '\n';

    std::thread b(addOneShared, std::ref(counter));  // the thread refers to counter itself
    b.join();
    std::cout << "after addOneShared: counter = " << counter << '\n';

    std::thread c([&counter] { counter *= 2; });  // a lambda capturing by reference
    c.join();
    std::cout << "after lambda:       counter = " << counter << '\n';

    auto order = std::make_unique<std::string>("soup for table 4");
    std::thread d(takeOwnership, std::move(order));  // ownership moves into the thread
    d.join();
    std::cout << "main's pointer is now " << (order ? "still set" : "empty") << '\n';
    return 0;
}
