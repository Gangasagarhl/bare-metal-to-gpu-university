// triage.cpp - F12-17 Listing 1: "compare good with bad". 6000 request records from a
// model incident are split by every attribute, then by pairs of attributes, to find the
// smallest description that covers the failures. The data are generated with a fixed seed.
#include <cstdio>
#include <random>
#include <string>
#include <vector>

struct Request
{
    int node;            // replica that served it: 1, 2 or 3
    std::string client;  // client library version
    char region;         // 'A' or 'B'
    int kib;             // payload size in KiB
    bool failed;
};

struct Tally
{
    long n = 0, bad = 0;
};

void row(const std::string& label, const Tally& t, double base)
{
    const double rate = t.n ? 100.0 * t.bad / t.n : 0.0;
    std::printf("  %-26s %5ld requests %4ld failed  %5.1f %%  lift %4.1f\n", label.c_str(), t.n,
                t.bad, rate, base > 0 ? rate / base : 0.0);
}

int main()
{
    std::mt19937 rng(1);  // the standard fixes mt19937's output, so the data are reproducible
    std::vector<Request> reqs;
    for (int i = 0; i < 6000; ++i) {
        Request r;
        r.node = static_cast<int>(rng() % 3) + 1;
        r.client = rng() % 100 < 40 ? "7.1" : "7.0";
        r.region = rng() % 2 ? 'A' : 'B';
        const unsigned large_pct = r.client == "7.1" ? 70 : 20;  // 7.1 sends bigger payloads
        const bool large = rng() % 100 < large_pct;
        r.kib = large ? 9 + static_cast<int>(rng() % 8) : 1 + static_cast<int>(rng() % 8);
        // The hidden cause: build b512 on n2 rejects payloads above 8 KiB.
        r.failed = (r.node == 2 && r.kib > 8) || rng() % 1000 < 2;
        reqs.push_back(r);
    }

    std::printf("what changed (deploy and rollout log)\n");
    std::printf("  day-1 09:00  client library 7.1 rollout begins (40 %% of clients by 15:00)\n");
    std::printf("  day-1 16:00  server build b512 deployed to n2 only (canary)\n");
    std::printf("  day-1 16:20  alert: error ratio above SLO burn threshold\n\n");

    Tally all;
    for (const Request& r : reqs) {
        all.n += 1;
        all.bad += r.failed;
    }
    const double base = 100.0 * all.bad / all.n;
    std::printf("1. all requests\n");
    row("everything", all, base);

    std::printf("\n2. one attribute at a time\n");
    Tally node[4], client70, client71, regA, regB, small, big;
    for (const Request& r : reqs) {
        Tally* ts[] = {&node[r.node], r.client == "7.1" ? &client71 : &client70,
                       r.region == 'A' ? &regA : &regB, r.kib > 8 ? &big : &small};
        for (Tally* t : ts) {
            t->n += 1;
            t->bad += r.failed;
        }
    }
    for (int n = 1; n <= 3; ++n) {
        row("node n" + std::to_string(n), node[n], base);
    }
    row("client 7.0", client70, base);
    row("client 7.1", client71, base);
    row("region A", regA, base);
    row("region B", regB, base);
    row("payload <= 8 KiB", small, base);
    row("payload > 8 KiB", big, base);

    std::printf("\n3. client version, split by node (is the client the cause?)\n");
    for (const std::string c : {"7.0", "7.1"}) {
        for (int n = 1; n <= 3; ++n) {
            Tally t;
            for (const Request& r : reqs) {
                if (r.client == c && r.node == n) {
                    t.n += 1;
                    t.bad += r.failed;
                }
            }
            row("client " + c + " on n" + std::to_string(n), t, base);
        }
    }

    std::printf("\n4. node n2 only, split by payload and client\n");
    for (const std::string c : {"7.0", "7.1"}) {
        for (const bool large : {false, true}) {
            Tally t;
            for (const Request& r : reqs) {
                if (r.node == 2 && r.client == c && (r.kib > 8) == large) {
                    t.n += 1;
                    t.bad += r.failed;
                }
            }
            row("n2, client " + c + (large ? ", > 8 KiB" : ", <= 8 KiB"), t, base);
        }
    }

    std::printf("\n5. how much does the candidate explanation cover?\n");
    Tally in, out;
    for (const Request& r : reqs) {
        Tally& t = (r.node == 2 && r.kib > 8) ? in : out;
        t.n += 1;
        t.bad += r.failed;
    }
    row("n2 and payload > 8 KiB", in, base);
    row("everything else", out, base);
    std::printf("  share of all failures inside the explanation: %.1f %%\n",
                100.0 * in.bad / all.bad);
    return 0;
}
