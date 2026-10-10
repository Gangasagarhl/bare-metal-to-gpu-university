#include <iostream>
#include <memory>

// Forensic evidence: the list is built fine, then crashes when it is destroyed.
struct Node
{
    int value = 0;
    std::unique_ptr<Node> next;           // default destructor: destroys next, which destroys next...
};

int main()
{
    std::cout << std::unitbuf;
    for (const int n : {1000, 1000000}) {
        std::unique_ptr<Node> head;
        for (int i = 0; i < n; ++i) {
            auto node = std::make_unique<Node>();
            node->value = i;
            node->next = std::move(head);
            head = std::move(node);
        }
        std::cout << "built a list of " << n << " nodes, now leaving the block\n";
    }
    std::cout << "done\n";
    return 0;
}
