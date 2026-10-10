// units_typed.cpp - SE301 F12-02: the fix for the forensic lab, moved from prose into the type.
// The interface takes std::chrono::microseconds, so the unit is part of the function's
// type. This file MUST NOT compile (units_typed.expect-fail): line 16 passes a bare number.
#include <chrono>

static bool rx_wait(std::chrono::microseconds deadline)
{
    return deadline.count() > 0;                     // body irrelevant: this is about the call
}

int main()
{
    using namespace std::chrono_literals;
    const bool a = rx_wait(5000us);                  // compiles: the unit is written
    const bool b = rx_wait(5ms);                     // compiles: converted exactly to 5000 us
    const bool c = rx_wait(5000);                    // error: a bare number has no unit
    return (a && b && c) ? 0 : 1;
}
