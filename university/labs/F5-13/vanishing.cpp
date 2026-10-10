// vanishing.cpp - generates the evidence pack of the F5-13 forensic lab "the comment that
// disappeared". Three replicas of a comment store; writes go to one replica and are copied to
// the others in the background; a load balancer sends each request to some replica.
// Clocks are synchronised in this scenario. The cause is explained only in the answer key.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Rng
{
    std::uint64_t state;
    std::uint64_t next()
    {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    int below(int n) { return static_cast<int>((next() >> 11) % static_cast<std::uint64_t>(n)); }
};

std::string at(int ms)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "10:15:%02d.%03d", ms / 1000, ms % 1000);
    return buf;
}

int main()
{
    // version of the thread "t42" on each replica at each moment: version 16 has 3 comments;
    // version 17 (Ana's new comment) is written at r1 at t = 1000 ms and copied to r2 after
    // 900 ms and to r3 after 4100 ms (r3 is behind: it is catching up after a restart).
    const int written = 1000;
    const int arrive[3] = {written, written + 900, written + 4100};
    auto version = [&](int replica, int t) { return t >= arrive[replica] ? 17 : 16; };

    Rng rng{3};
    std::printf("=== web.log (load balancer: any replica; store settings: replicas=3, "
                "write_acks=1, read_from=1) ===\n");
    std::printf("%s ana  POST /thread/t42/comment  -> r1  stored version 17  200 OK\n",
                at(written).c_str());
    struct Req { int t; const char* user; };
    const std::vector<Req> reqs = {{1350, "ana"}, {1800, "ben"}, {2300, "ana"}, {2900, "ana"},
                                   {3600, "ben"}, {4200, "ana"}, {5600, "ana"}};
    std::vector<std::string> history;
    history.push_back("init w 16 0 1");
    history.push_back("ana w 17 1000 1000");
    for (const Req& q : reqs) {
        int r = rng.below(3);
        if (q.t == 1350) {
            r = 0;                         // (the balancer's choices for these three requests)
        } else if (q.t == 2300 || q.t == 4200) {
            r = 2;
        }
        const int v = version(r, q.t);
        std::printf("%s %-4s GET  /thread/t42           -> r%d  version %d  comments %d\n",
                    at(q.t).c_str(), q.user, r + 1, v, v == 17 ? 4 : 3);
        history.push_back(std::string(q.user) + " r " + std::to_string(v) + " "
                          + std::to_string(q.t) + " " + std::to_string(q.t + 20));
    }
    std::printf("\n=== replication.log ===\n");
    for (int r = 1; r < 3; ++r) {
        std::printf("%s r%d applied t42 version 17 (from r1)\n", at(arrive[r]).c_str(), r + 1);
    }
    std::printf("\n=== support ticket ===\n");
    std::printf("\"I posted my comment at 10:15:01, saw it, reloaded and it was GONE, then it "
                "came back. Ben says he never saw it vanish.\" (ana)\n");
    std::printf("\n=== the same requests as a history for histories.cpp (value = version) ===\n");
    for (const std::string& h : history) {
        std::printf("%s\n", h.c_str());
    }
    return 0;
}
