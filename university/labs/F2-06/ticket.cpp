#include <iostream>
#include <string>
#include <vector>

// One order ticket: four named fields that travel together.
struct Order
{
    int table = 0;
    std::string dish;
    int quantity = 1;
    double price_each = 0.0;
};

double order_total(const Order& order)
{
    return order.quantity * order.price_each;
}

void print_ticket(const Order& order)
{
    std::cout << "Table " << order.table << ": " << order.quantity << " x " << order.dish
              << " = " << order_total(order) << '\n';
}

void add_one_more(Order& order)
{
    ++order.quantity;                    // changes the caller's ticket (reference)
}

Order make_tea_order(int table)
{
    Order tea;                           // starts with the default values
    tea.table = table;
    tea.dish = "tea";
    tea.price_each = 1.5;
    return tea;                          // a struct can be returned like an int
}

int main()
{
    Order soup{.table = 2, .dish = "soup", .quantity = 3, .price_each = 4.0};
    Order rice{.table = 5, .dish = "rice", .price_each = 2.5};   // quantity keeps its default 1
    add_one_more(rice);

    std::vector<Order> open_orders{soup, rice, make_tea_order(7)};
    double evening = 0.0;
    for (const Order& order : open_orders) {
        print_ticket(order);
        evening += order_total(order);
    }
    std::cout << "Open orders: " << open_orders.size() << ", total " << evening << '\n';
    return 0;
}
