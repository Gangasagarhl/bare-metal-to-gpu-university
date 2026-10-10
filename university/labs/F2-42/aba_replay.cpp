// Listing 3 (F2-42): the ABA problem, replayed step by step in ONE thread so that the
// dangerous interleaving happens every time. Nodes are reused from a free list, as an
// allocator might do. "Thread A" and "thread B" are the two halves of the story.
#include <atomic>
#include <iostream>
#include <string>

struct Node
{
    std::string dish;
    Node* next;
};

std::atomic<Node*> top{nullptr};

void show(const std::string& when)
{
    std::cout << when << ": stack = [";
    for (Node* n = top.load(); n != nullptr; n = n->next) {
        std::cout << n->dish << (n->next ? ", " : "");
    }
    std::cout << "]\n";
}

int main()
{
    Node x{"X soup", nullptr};
    Node y{"Y rice", nullptr};
    Node z{"Z tea", nullptr};
    x.next = &y;
    y.next = &z;
    top.store(&x);
    show("start                      ");

    // Thread A starts pop(): reads top and top->next, then is paused by the scheduler.
    Node* aOld = top.load();
    Node* aNext = aOld->next;
    std::cout << "A reads top = " << aOld->dish << ", next = " << aNext->dish << ", then pauses\n";

    // Thread B runs: pops X, pops Y, then pushes X back (the same node, reused).
    top.store(x.next);  // B pops X
    show("B popped X                 ");
    top.store(y.next);  // B pops Y and reuses the node for a new order elsewhere
    y.dish = "Y reused: bread, table 9";
    y.next = nullptr;
    show("B popped Y and reused it   ");
    x.next = top.load();
    top.store(&x);      // B pushes X again: top is X once more
    show("B pushed X back            ");

    // Thread A resumes: its CAS compares top with X. It IS X, so the CAS succeeds...
    Node* expected = aOld;
    const bool ok = top.compare_exchange_strong(expected, aNext);
    std::cout << "A's CAS(top: X -> " << aNext->dish << ") " << (ok ? "SUCCEEDED" : "failed") << '\n';
    show("after A's pop              ");
    std::cout << "A returns " << aOld->dish << ". The correct result was [" << z.dish << "]: instead the stack holds a node"
              << " B had already taken, and " << z.dish << " is lost\n";
    return 0;
}
