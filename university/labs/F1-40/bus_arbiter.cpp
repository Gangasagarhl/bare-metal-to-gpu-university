// F1-40 Listing 2: two bus masters share one bus. Each cycle the arbiter grants
// the bus to exactly one requester (round-robin), so the other one waits.
#include <cstdio>
#include <string>
#include <vector>

struct Master
{
    std::string name;
    int wanted;      // transactions this master still has to do
    int done = 0;
    int waited = 0;  // cycles spent requesting but not granted
};

int main()
{
    std::vector<Master> m{{"CPU", 6}, {"DMA", 4}};
    int last = 1;    // index of the master granted last time
    int cycle = 0;
    while (m[0].done < m[0].wanted || m[1].done < m[1].wanted) {
        ++cycle;
        const bool req0 = m[0].done < m[0].wanted;
        const bool req1 = m[1].done < m[1].wanted;
        int grant = -1;
        if (req0 && req1) {
            grant = (last == 0) ? 1 : 0;   // both ask: take turns
        } else if (req0) {
            grant = 0;
        } else {
            grant = 1;
        }
        last = grant;
        ++m[grant].done;
        const int other = 1 - grant;
        const bool other_req = (other == 0) ? req0 : req1;
        if (other_req) {
            ++m[other].waited;
        }
        std::printf("cycle %2d  request CPU=%d DMA=%d  grant=%s%s\n", cycle, req0 ? 1 : 0,
                    req1 ? 1 : 0, m[grant].name.c_str(), other_req ? "  (other waits)" : "");
    }
    for (const Master& x : m) {
        std::printf("%s: %d transactions, %d cycles waiting\n", x.name.c_str(), x.done, x.waited);
    }
    std::printf("bus busy for %d cycles: the work of both masters is serialised\n", cycle);
    return 0;
}
