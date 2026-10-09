// Explore every delivery order of two tiny consensus protocols for three
// processes with inputs 0 or 1, as the FLP paper's model allows: messages can
// be delayed arbitrarily, and one process may be silent (crashed) from the start.
#include <cstdio>
#include <set>
#include <string>
#include <vector>

constexpr int kN = 3;

struct Message
{
    int from;
    int to;
    int value;
};

struct Config
{
    std::vector<int> input;            // input[p]
    std::vector<std::vector<int>> got; // got[p]: values p holds (its own first)
    std::vector<int> decision;         // -1: undecided
    std::vector<Message> buffer;       // sent, not yet delivered
};

enum class Protocol { WaitForAll, WaitForTwo };

// The decision rule, applied whenever p holds a new value.
static int decide(Protocol proto, const std::vector<int>& got)
{
    if (proto == Protocol::WaitForAll) {
        if (static_cast<int>(got.size()) < kN) {
            return -1;
        }
        int ones = 0;
        for (int v : got) {
            ones += v;
        }
        return 2 * ones > kN ? 1 : 0;  // majority of the three inputs
    }
    if (got.size() < 2) {  // WaitForTwo: own value and the first one to arrive
        return -1;
    }
    return got[0] == 1 && got[1] == 1 ? 1 : 0;  // a tie goes to 0
}

struct Summary
{
    bool decided0 = false;   // in some schedule, some process decides 0
    bool decided1 = false;
    bool disagree = false;   // in some schedule, two processes decide differently
    bool blocks = false;     // in some schedule, a live process never decides
    long schedules = 0;
};

static void explore(Protocol proto, Config c, int silent, Summary& s)
{
    bool moved = false;
    for (std::size_t i = 0; i < c.buffer.size(); ++i) {
        const Message m = c.buffer[i];
        if (m.from == silent || m.to == silent || c.decision[m.to] >= 0) {
            continue;  // never delivered, or no longer matters
        }
        Config next = c;
        next.buffer.erase(next.buffer.begin() + static_cast<long>(i));
        next.got[m.to].push_back(m.value);
        next.decision[m.to] = decide(proto, next.got[m.to]);
        moved = true;
        explore(proto, next, silent, s);
    }
    if (moved) {
        return;
    }
    ++s.schedules;  // a final state: nothing left that could change anything
    std::set<int> values;
    for (int p = 0; p < kN; ++p) {
        if (p == silent) {
            continue;
        }
        if (c.decision[p] < 0) {
            s.blocks = true;
        } else {
            values.insert(c.decision[p]);
        }
    }
    s.decided0 = s.decided0 || values.count(0);
    s.decided1 = s.decided1 || values.count(1);
    s.disagree = s.disagree || values.size() == 2;
}

static Summary run(Protocol proto, int inputs, int silent)
{
    Config c;
    for (int p = 0; p < kN; ++p) {
        c.input.push_back(inputs >> (kN - 1 - p) & 1);
        c.got.push_back({c.input[p]});
        c.decision.push_back(-1);
    }
    for (int p = 0; p < kN; ++p) {
        for (int q = 0; q < kN; ++q) {
            if (p != q) {
                c.buffer.push_back({p, q, c.input[p]});
            }
        }
    }
    Summary s;
    explore(proto, c, silent, s);
    return s;
}

static std::string cell(const Summary& s)
{
    std::string t;
    if (s.decided0 && s.decided1) {
        t = "0/1";
    } else if (s.decided0) {
        t = "0";
    } else if (s.decided1) {
        t = "1";
    } else {
        t = "-";
    }
    if (s.disagree) {
        t += "!";
    }
    if (s.blocks && (s.decided0 || s.decided1)) {
        t += "~";
    }
    return t;
}

static void table(Protocol proto, const char* title)
{
    std::printf("%s\n                ", title);
    for (int in = 0; in < 8; ++in) {
        std::printf(" %d%d%d   ", in >> 2 & 1, in >> 1 & 1, in & 1);
    }
    std::printf("\n");
    long total = 0;
    for (int silent = -1; silent < kN; ++silent) {
        if (silent < 0) {
            std::printf("  nobody silent ");
        } else {
            std::printf("  p%d silent     ", silent + 1);
        }
        for (int in = 0; in < 8; ++in) {
            const Summary s = run(proto, in, silent);
            total += s.schedules;
            std::printf(" %-6s", cell(s).c_str());
        }
        std::printf("\n");
    }
    std::printf("  (%ld complete delivery orders explored)\n\n", total);
}

int main()
{
    std::printf("Columns: inputs of p1 p2 p3. Cell: decision values reachable over all\n"
                "delivery orders; 0/1 = bivalent; ! = two processes can decide differently;\n"
                "~ = some order leaves a live process undecided; - = nobody ever decides.\n\n");
    table(Protocol::WaitForAll, "Protocol W3: wait for all three values, decide the majority");
    table(Protocol::WaitForTwo, "Protocol W2: wait for own value + one more, decide (tie -> 0)");

    // The chain argument (FLP Lemma 2) on W3 without failures.
    std::printf("Chain 000 -> 100 -> 110 -> 111 for W3, nobody silent:\n");
    const int chain[] = {0b000, 0b100, 0b110, 0b111};
    for (int i = 0; i < 4; ++i) {
        const Summary s = run(Protocol::WaitForAll, chain[i], -1);
        std::printf("  %d%d%d : %s\n", chain[i] >> 2 & 1, chain[i] >> 1 & 1, chain[i] & 1,
                    cell(s).c_str());
    }
    std::printf("  100 and 110 differ only in p2's input but decide differently.\n");
    const Summary a = run(Protocol::WaitForAll, 0b100, 1);
    const Summary b = run(Protocol::WaitForAll, 0b110, 1);
    std::printf("  With p2 silent: 100 -> %s, 110 -> %s (W3 waits forever)\n", cell(a).c_str(),
                cell(b).c_str());
    const Summary c = run(Protocol::WaitForTwo, 0b100, 1);
    const Summary d = run(Protocol::WaitForTwo, 0b110, 1);
    std::printf("  W2 with p2 silent: 100 -> %s, 110 -> %s (same, as it must be)\n",
                cell(c).c_str(), cell(d).c_str());
    return 0;
}
