// lockorder.cpp - a tiny lock-order checker on the host (the idea behind the kernel's
// lockdep_check_order): remember "A was held while B was taken"; report the reverse.
#include <cstdio>
#include <set>
#include <string>
#include <utility>
#include <vector>

class OrderChecker {
public:
    // Called before a thread waits for 'lock' while it holds 'held'.
    void acquire(const std::vector<std::string>& held, const std::string& lock)
    {
        for (const std::string& h : held) {
            if (edges_.count({lock, h})) {
                std::printf("LOCK ORDER INVERSION: taking '%s' while holding '%s',\n"
                            "  but earlier '%s' was held while taking '%s'\n",
                            lock.c_str(), h.c_str(), lock.c_str(), h.c_str());
                ++reports_;
            }
            edges_.insert({h, lock});
        }
    }
    int reports() const { return reports_; }

private:
    std::set<std::pair<std::string, std::string>> edges_;   // (first held, then taken)
    int reports_ = 0;
};

int main()
{
    OrderChecker chk;
    // Thread 1 runs transfer(alice, bob): it takes alice, then bob.
    chk.acquire({}, "account:alice");
    chk.acquire({"account:alice"}, "account:bob");
    std::printf("transfer(alice -> bob): %d reports\n", chk.reports());
    // Later, thread 2 runs transfer(bob, alice). On one CPU the two never overlap,
    // so no deadlock happens - but the order graph already has a cycle.
    chk.acquire({}, "account:bob");
    chk.acquire({"account:bob"}, "account:alice");
    std::printf("transfer(bob -> alice): %d reports\n", chk.reports());
    // The fix: always take the two locks in one global order (here: by name).
    OrderChecker fixed;
    auto ordered = [&](std::string a, std::string b) {
        if (b < a) std::swap(a, b);
        fixed.acquire({}, a);
        fixed.acquire({a}, b);
    };
    ordered("account:alice", "account:bob");
    ordered("account:bob", "account:alice");
    std::printf("with ordered locking: %d reports\n", fixed.reports());
    return 0;
}
