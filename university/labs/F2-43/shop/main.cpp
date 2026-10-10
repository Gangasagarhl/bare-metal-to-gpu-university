// main.cpp: USES names that another translation unit defines.
#include <cstdio>

extern int g_orders;     // declared here, defined in counter.cpp
int next_order_id();     // declared here, defined in counter.cpp
int calls_so_far();

int main()
{
    const int a = next_order_id();
    const int b = next_order_id();
    std::printf("orders %d and %d, counter now %d, calls %d\n", a, b, g_orders, calls_so_far());
    return 0;
}
