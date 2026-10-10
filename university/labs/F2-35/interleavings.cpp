// Worked-example check (F2-35): every interleaving of two threads that each run
//   r = x;  r = r + 1;  x = r;
// starting from x = 0. Counts how many orders give x == 2 and how many lose an update.
#include <iostream>
#include <string>

struct Machine
{
    int x = 0;
    int r[2] = {0, 0};
    int step[2] = {0, 0};
};

void runStep(Machine& m, int t)
{
    switch (m.step[t]) {
    case 0: m.r[t] = m.x; break;          // load
    case 1: m.r[t] = m.r[t] + 1; break;   // add
    case 2: m.x = m.r[t]; break;          // store
    }
    ++m.step[t];
}

int total = 0;
int lost = 0;

void explore(Machine m, const std::string& order)
{
    if (m.step[0] == 3 && m.step[1] == 3) {
        ++total;
        if (m.x != 2) {
            ++lost;
            if (lost <= 3) {
                std::cout << "lost update, order " << order << " -> x = " << m.x << '\n';
            }
        }
        return;
    }
    for (int t = 0; t < 2; ++t) {
        if (m.step[t] < 3) {
            Machine next = m;
            runStep(next, t);
            explore(next, order + (t == 0 ? 'A' : 'B'));
        }
    }
}

int main()
{
    explore(Machine{}, "");
    std::cout << "interleavings: " << total << ", correct (x == 2): " << total - lost
              << ", lost an update (x == 1): " << lost << '\n';
    return 0;
}
