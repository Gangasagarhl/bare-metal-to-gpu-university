#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Lab reference solution: an order book that copies deeply and moves cheaply.
// Its own promise for the moved-from state: an OrderBook that was moved from is empty.
class OrderBook
{
public:
    OrderBook() = default;
    OrderBook(const OrderBook&) = default;
    OrderBook& operator=(const OrderBook&) = default;

    OrderBook(OrderBook&& other) noexcept : orders_(other.orders_)  // lab step 3: no std::move
    {
    }

    OrderBook& operator=(OrderBook&& other) noexcept
    {
        if (this != &other) {
            orders_ = std::move(other.orders_);
            other.orders_.clear();
        }
        return *this;
    }

    void add(const std::string& order) { orders_.push_back(order); }
    std::size_t size() const { return orders_.size(); }
    const std::string& at(std::size_t i) const { return orders_.at(i); }

private:
    std::vector<std::string> orders_;
};

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

int main()
{
    OrderBook lunch;
    lunch.add("soup");
    lunch.add("rice");

    OrderBook copy = lunch;
    copy.add("tea");
    expect(lunch.size() == 2 && copy.size() == 3, "a copy is independent");

    OrderBook moved = std::move(copy);
    expect(moved.size() == 3 && moved.at(2) == "tea", "move hands over every order");
    expect(copy.size() == 0, "moved-from book is empty (our own promise)");

    copy = lunch;
    expect(copy.size() == 2, "a moved-from book can be assigned again");
    OrderBook& same = lunch;        // self-move through a second name, as it happens in real code
    lunch = std::move(same);
    expect(lunch.size() == 2, "self-move-assignment keeps the orders");
    std::cout << "OrderBook tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
