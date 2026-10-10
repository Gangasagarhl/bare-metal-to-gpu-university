// sensor.cpp (library code): reads a temperature and calls the board's alarm hook.
#include <cstdio>

// Default hook. "weak" lets a board file supply its own strong definition that wins at link time.
__attribute__((weak)) void on_overheat(int celsius)
{
    std::printf("default on_overheat(%d): nothing to do\n", celsius);
}

void check_temperature(int celsius)
{
    if (celsius > 90) {
        on_overheat(celsius);
    }
}
