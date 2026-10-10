// pin_attempt.cpp - try to pin down today's behaviour with a plain test. It cannot be done:
// the same input, one second apart, gives two different outputs.
#include <chrono>
#include <iostream>
#include <thread>

#include "legacy_status.hpp"

int main()
{
    const std::string a = legacy::formatStatus("V=12.34;I=1.5;T=30");
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    const std::string b = legacy::formatStatus("V=12.34;I=1.5;T=30");
    std::cout << "first call:  " << a << "\nsecond call: " << b << "\n"
              << "identical: " << (a == b ? "yes" : "no") << "\n";
    return 0;
}
