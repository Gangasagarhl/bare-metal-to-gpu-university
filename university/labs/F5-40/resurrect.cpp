// resurrect.cpp - generates the evidence pack of the F5-40 forensic lab "the stove came back".
// A Dynamo-style store (N=3, R=2, W=2, vector clocks, siblings returned to the application)
// holds a shared packing list. The application merges siblings with its own function. The
// store and the application really run here: vector clocks are compared by the code below,
// and the logs are what the components printed. Clocks are synchronised in this scenario.
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

using Clock = std::map<std::string, int>;   // vector clock: coordinator -> counter
using Items = std::set<std::string>;

struct Version
{
    Clock vc;
    Items items;
};

std::string str(const Clock& c)
{
    std::string s = "[";
    for (const auto& [n, k] : c) {
        s += (s.size() > 1 ? " " : "") + n + ":" + std::to_string(k);
    }
    return s + "]";
}

std::string str(const Items& it)
{
    std::string s = "{";
    for (const auto& x : it) {
        s += (s.size() > 1 ? "," : "") + x;
    }
    return s + "}";
}

bool descends(const Clock& a, const Clock& b)   // a >= b in every entry
{
    for (const auto& [n, k] : b) {
        auto it = a.find(n);
        if (it == a.end() || it->second < k) {
            return false;
        }
    }
    return true;
}

struct Replica
{
    std::string name;
    std::vector<Version> versions;   // siblings: mutually concurrent versions
    std::vector<std::string> log;

    void put(const std::string& t, const Version& v)
    {
        std::vector<Version> keep;
        for (const auto& old : versions) {
            if (!descends(v.vc, old.vc)) {
                keep.push_back(old);   // concurrent with the new one: keep as a sibling
            }
        }
        bool dominated = false;
        for (const auto& old : keep) {
            dominated = dominated || descends(old.vc, v.vc);
        }
        if (!dominated) {
            keep.push_back(v);
        }
        versions = keep;
        std::string s = t + " " + name + ": store " + str(v.vc) + " " + str(v.items) + " -> holds";
        for (const auto& x : versions) {
            s += " " + str(x.vc);
        }
        log.push_back(s);
    }
};

int main()
{
    std::map<std::string, Replica> r;
    for (const char* n : {"A", "B", "C"}) {
        r[n].name = n;
    }
    std::vector<std::string> app;
    std::vector<std::string> net;

    // 17:40 the list is created through coordinator A
    Version v{{{"A", 3}}, {"lamp", "map", "stove", "tent"}};
    for (const char* n : {"A", "B", "C"}) {
        r[n].put("17:40:00", v);
    }
    app.push_back("17:40:00 leila: put list " + str(v.items) + " -> ok " + str(v.vc));

    // 17:50 Leila removes the lamp; nobody else writes: an ordinary update
    Version v2{{{"A", 4}}, {"map", "stove", "tent"}};
    for (const char* n : {"A", "B", "C"}) {
        r[n].put("17:50:00", v2);
    }
    app.push_back("17:50:00 leila: get list -> 1 version " + str(v.vc) + "; remove lamp; put -> ok "
                  + str(v2.vc));

    // 18:00 the network splits: {A, B} | {C}
    net.push_back("18:00:00 partition: {A,B} cannot reach {C}");
    Version leila{{{"A", 5}}, {"map", "tent"}};
    r["A"].put("18:01:10", leila);
    r["B"].put("18:01:10", leila);
    app.push_back("18:01:10 leila: get list -> 1 version " + str(v2.vc) + "; remove stove; put via A -> ok "
                  + str(leila.vc));
    Version omar{{{"A", 4}, {"C", 1}}, {"map", "rope", "stove", "tent"}};
    r["C"].put("18:02:30", omar);
    app.push_back("18:02:30 omar: get list -> 1 version " + str(v2.vc) + "; add rope; put via C"
                  " (sloppy quorum, hinted copy on D) -> ok " + str(omar.vc));
    net.push_back("18:20:00 partition healed; anti-entropy and hinted handoff exchange versions");
    for (const char* n : {"A", "B"}) {
        r[n].put("18:20:05", omar);
    }
    r["C"].put("18:20:05", leila);

    // 18:30 Leila opens the list again: the store returns both siblings; the app merges them
    std::string s = "18:30:00 leila: get list -> " + std::to_string(r["A"].versions.size()) + " versions:";
    Items merged;
    Clock mc;
    for (const auto& x : r["A"].versions) {
        s += " " + str(x.vc) + " " + str(x.items);
        merged.insert(x.items.begin(), x.items.end());   // mergeSiblings(): union of the item sets
        for (const auto& [n, k] : x.vc) {
            mc[n] = std::max(mc[n], k);
        }
    }
    app.push_back(s);
    mc["A"] += 1;
    Version m{mc, merged};
    for (const char* n : {"A", "B", "C"}) {
        r[n].put("18:30:01", m);
    }
    app.push_back("18:30:01 leila: mergeSiblings -> " + str(merged) + "; put via A -> ok " + str(mc));
    app.push_back("18:31:40 leila: (support ticket) 'I removed the stove at 18:01 and it is back on the list'");

    std::printf("=== store configuration (excerpt) ===\n");
    std::printf("N = 3   R = 2   W = 2   sloppy_quorum = on   conflict_resolution = return_siblings_to_client\n");
    std::printf("app: on_siblings = mergeSiblings   # \"union of the item sets, so that no added item is lost\"\n\n");
    std::printf("=== network.log ===\n");
    for (const auto& l : net) {
        std::printf("%s\n", l.c_str());
    }
    std::printf("\n=== app.log ===\n");
    for (const auto& l : app) {
        std::printf("%s\n", l.c_str());
    }
    for (const char* n : {"A", "C"}) {
        std::printf("\n=== replica-%s.log ===\n", n);
        for (const auto& l : r[n].log) {
            std::printf("%s\n", l.c_str());
        }
    }
    return 0;
}
