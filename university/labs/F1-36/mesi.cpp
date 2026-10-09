// A snooping MESI coherence model: every core has a private cache, all caches watch one bus.
// Input lines: "<core> R|W <hex block address>". Caches are large enough never to evict.
// Output: the bus action of each access and the state of that block in every cache afterwards.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

enum class State { I, S, E, M };

char letter(State s)
{
    switch (s) {
    case State::M: return 'M';
    case State::E: return 'E';
    case State::S: return 'S';
    default: return 'I';
    }
}

class System
{
public:
    explicit System(int cores) : caches_(cores) {}

    std::string access(int core, char op, std::uint64_t block)
    {
        State& mine = caches_[core][block];                // absent blocks start as I
        std::string bus = "-";
        bool othersHaveIt = false;
        for (int c = 0; c < static_cast<int>(caches_.size()); ++c) {
            if (c != core && caches_[c][block] != State::I) {
                othersHaveIt = true;
            }
        }
        if (op == 'R' && mine == State::I) {
            bus = "BusRd";
            for (int c = 0; c < static_cast<int>(caches_.size()); ++c) {
                State& other = caches_[c][block];
                if (c == core || other == State::I) {
                    continue;
                }
                if (other == State::M) {
                    bus += " + owner writes back";             // memory gets the new value
                }
                other = State::S;
            }
            mine = othersHaveIt ? State::S : State::E;
        } else if (op == 'W' && mine != State::M) {
            if (mine == State::E) {
                bus = "- (silent E->M)";
            } else {
                bus = mine == State::S ? "BusUpgr" : "BusRdX";
                for (int c = 0; c < static_cast<int>(caches_.size()); ++c) {
                    State& other = caches_[c][block];
                    if (c == core || other == State::I) {
                        continue;
                    }
                    if (other == State::M) {
                        bus += " + owner writes back";
                    }
                    other = State::I;                          // every other copy is invalidated
                }
            }
            mine = State::M;
        }
        if (bus != "-" && bus.rfind("- ", 0) != 0) {
            ++busTransactions_;
        }
        return bus;
    }

    std::string states(std::uint64_t block)
    {
        std::string s;
        for (auto& cache : caches_) {
            s += letter(cache[block]);
            s += ' ';
        }
        return s;
    }

    int busTransactions() const { return busTransactions_; }

private:
    std::vector<std::map<std::uint64_t, State>> caches_;
    int busTransactions_ = 0;
};

int main()
{
    int cores = 0;
    std::cin >> cores;
    System sys(cores);
    std::printf("%3s %5s %3s %6s  %-30s %s\n", "#", "core", "op", "block", "bus",
                "states (core 0..)");
    int core = 0;
    char op = 0;
    std::string word;
    int n = 0;
    while (std::cin >> core >> op >> word) {
        const std::uint64_t block = std::stoull(word, nullptr, 16);
        const std::string bus = sys.access(core, op, block);
        std::printf("%3d %5d %3c %6llx  %-30s %s\n", ++n, core, op,
                    static_cast<unsigned long long>(block), bus.c_str(), sys.states(block).c_str());
    }
    std::printf("bus transactions: %d\n", sys.busTransactions());
    return 0;
}
