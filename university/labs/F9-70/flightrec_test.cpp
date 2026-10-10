// flightrec_test.cpp - F9-70: check the flight recorder's wrap-around before trusting it.
#include <iostream>

#include "flightrec.h"

int main()
{
    FlightRecorder<4> rec;
    for (uint32_t i = 0; i < 10; ++i) {
        rec.push(Record{int64_t{i} * 1000, i, static_cast<int32_t>(i), Ev::Cycle});
    }
    std::cout << "written " << rec.written() << ", overwritten " << rec.overwritten() << ", held:";
    uint32_t expect = 6;
    bool ok = true;
    rec.for_each([&](const Record& r) {
        std::cout << ' ' << r.cycle;
        ok = ok && r.cycle == expect++;
    });
    std::cout << '\n' << (ok && rec.overwritten() == 6 ? "PASS" : "FAIL") << ": the ring keeps the last 4 records, oldest first\n";
    return ok ? 0 : 1;
}
