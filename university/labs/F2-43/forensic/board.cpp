// board.cpp (board support): the real alarm hook, which must switch the heater off.
#include <cstdio>

void on_overheat(int celsius)
{
    std::printf("board on_overheat(%d): HEATER OFF\n", celsius);
}
