// Listing 4 (F2-34): a std::thread destroyed while still joinable ends the program.
#include <iostream>
#include <thread>

void work()
{
}

int main()
{
    std::cout << "starting a worker and forgetting to join it" << std::endl;
    {
        std::thread worker(work);
    }  // ~thread() on a joinable thread calls std::terminate()
    std::cout << "this line is never printed" << std::endl;
    return 0;
}
