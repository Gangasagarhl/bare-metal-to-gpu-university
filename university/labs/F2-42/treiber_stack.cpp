// Listing 2 (F2-42): a lock-free stack (Treiber's algorithm) built on compare_exchange.
// Simplification on purpose: popped nodes are NOT freed while threads run (no memory
// reclamation); they are kept and deleted after all threads have finished.
#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

struct Node
{
    int value;
    Node* next;
};

class LockFreeStack
{
public:
    void push(Node* n)
    {
        n->next = top_.load(std::memory_order_relaxed);
        while (!top_.compare_exchange_weak(n->next, n, std::memory_order_release, std::memory_order_relaxed)) {
            // CAS failed: n->next now holds the current top; try again
        }
    }

    Node* pop()
    {
        Node* old = top_.load(std::memory_order_acquire);
        while (old != nullptr &&
               !top_.compare_exchange_weak(old, old->next, std::memory_order_acquire, std::memory_order_acquire)) {
            // CAS failed: old now holds the current top; try again
        }
        return old;  // nullptr if the stack was empty
    }

private:
    std::atomic<Node*> top_{nullptr};
};

int main()
{
    LockFreeStack stack;
    const int perThread = 50'000;
    std::vector<std::unique_ptr<Node>> owned(4 * perThread);  // owns every node; freed at the end
    for (int i = 0; i < 4 * perThread; ++i) {
        owned[i] = std::make_unique<Node>(Node{i, nullptr});
    }
    std::vector<long> poppedSum(4, 0);
    std::vector<int> poppedCount(4, 0);
    std::vector<std::thread> ts;
    for (int t = 0; t < 4; ++t) {
        ts.emplace_back([&, t] {
            for (int i = 0; i < perThread; ++i) {
                stack.push(owned[t * perThread + i].get());
                if (i % 2 == 1) {  // pop after every second push
                    if (Node* n = stack.pop()) {
                        poppedSum[t] += n->value;
                        ++poppedCount[t];
                    }
                }
            }
        });
    }
    for (auto& th : ts) {
        th.join();
    }
    long sum = 0;
    int count = 0;
    for (int t = 0; t < 4; ++t) {
        sum += poppedSum[t];
        count += poppedCount[t];
    }
    while (Node* n = stack.pop()) {  // drain what is left, single-threaded
        sum += n->value;
        ++count;
    }
    const long expected = static_cast<long>(4 * perThread) * (4 * perThread - 1) / 2;
    std::cout << "nodes popped: " << count << " of " << 4 * perThread << ", sum " << sum << " (expected " << expected
              << ")\n";
    std::cout << (count == 4 * perThread && sum == expected ? "PASS" : "FAIL") << '\n';
    return 0;
}
