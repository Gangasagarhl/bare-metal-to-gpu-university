#include <cassert>
#include <iostream>

int orders_sent = 0;

bool send_order_to_kitchen()
{
    ++orders_sent;
    return true;
}

int main()
{
    assert(send_order_to_kitchen());     // the call is inside the assert: a trap
    std::cout << "orders sent: " << orders_sent << '\n';
    return 0;
}
