#include <cassert>
#include <iostream>
#include <memory>
#include <string>

// A singly linked list in which every node OWNS the next one through a unique_ptr.
class OrderList
{
public:
    void pushFront(std::string dish)
    {
        auto node = std::make_unique<Node>();
        node->dish = std::move(dish);
        node->next = std::move(head_);      // the new node takes over the old chain
        head_ = std::move(node);            // the list owns the new node
        ++size_;
    }

    std::string popFront()
    {
        assert(head_ != nullptr);
        std::string dish = std::move(head_->dish);
        head_ = std::move(head_->next);     // the old head is destroyed here
        --size_;
        return dish;
    }

    std::size_t size() const { return size_; }

    void reverse()                          // re-links the nodes; no node is copied or freed
    {
        std::unique_ptr<Node> done;
        while (head_) {
            std::unique_ptr<Node> rest = std::move(head_->next);
            head_->next = std::move(done);
            done = std::move(head_);
            head_ = std::move(rest);
        }
        head_ = std::move(done);
    }

    void print() const
    {
        for (const Node* n = head_.get(); n != nullptr; n = n->next.get()) {
            std::cout << n->dish << (n->next ? " -> " : "\n");
        }
    }

    ~OrderList()
    {
        while (head_) {                     // iterative: no deep chain of destructor calls
            head_ = std::move(head_->next);
        }
    }

private:
    struct Node
    {
        std::string dish;
        std::unique_ptr<Node> next;
    };
    std::unique_ptr<Node> head_;
    std::size_t size_ = 0;
};

int main()
{
    OrderList orders;
    orders.pushFront("tea");
    orders.pushFront("soup");
    orders.pushFront("rice");
    orders.print();
    assert(orders.size() == 3);
    assert(orders.popFront() == "rice");
    assert(orders.size() == 2);
    orders.print();
    orders.pushFront("bread");
    orders.reverse();
    orders.print();
    assert(orders.popFront() == "tea");

    OrderList big;                          // a long list must also be destroyed safely
    for (int i = 0; i < 200000; ++i) {
        big.pushFront("x");
    }
    std::cout << "big list size " << big.size() << '\n';
    std::cout << "all tests passed\n";
    return 0;
}
