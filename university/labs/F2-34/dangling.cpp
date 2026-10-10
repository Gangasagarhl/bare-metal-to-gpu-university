// Forensic program (F2-34): "The order list that vanished".
// A detached worker keeps a reference to a vector that is destroyed before it reads it.
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

void printLater(const std::vector<int>& tables)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "first table: " << tables.front() << std::endl;
}

void startPrinter()
{
    std::vector<int> tables = {2, 5, 7};
    std::thread printer(printLater, std::cref(tables));
    printer.detach();  // nobody waits for the printer
}  // tables is destroyed here, while the printer is still sleeping

int main()
{
    startPrinter();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "main finished" << std::endl;
    return 0;
}
