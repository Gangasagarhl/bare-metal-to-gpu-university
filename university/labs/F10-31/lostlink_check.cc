// lostlink_check.cc - F10-31 forensic ANSWER-KEY tool: read the "Lost link" evidence
// (lostlink_gen.out), list the gaps in each heartbeat stream with what the sequence
// numbers say about them, and replay the failsafe rule (timeout 5.0 s) under two policies:
// any ground heartbeat keeps the vehicle happy, or only the controlling ground station's.
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct Hb {
    double t;
    int seq;
};

void parseCell(const std::string& cell, std::vector<Hb>& out)
{
    std::istringstream in(cell);
    std::string item;
    while (in >> item) {
        const auto hash = item.find('#');
        if (hash != std::string::npos) {
            out.push_back({std::stod(item.substr(0, hash)), std::stoi(item.substr(hash + 1))});
        }
    }
}

void gaps(const char* name, const std::vector<Hb>& v, int seqStep)
{
    std::printf("%s: %zu heartbeats\n", name, v.size());
    for (std::size_t i = 1; i < v.size(); ++i) {
        const double dt = v[i].t - v[i - 1].t;
        if (dt > 1.5) {
            const int dseq = (v[i].seq - v[i - 1].seq + 256) % 256;
            const int lost = dseq / seqStep - 1;
            std::printf("  gap %.2f s from %.2f to %.2f; seq %d -> %d: %s\n", dt, v[i - 1].t,
                        v[i].t, v[i - 1].seq, v[i].seq,
                        lost > 0 ? (std::to_string(lost) + " sent but not received").c_str()
                                 : "nothing was sent (the sender stopped)");
        }
    }
}

// first time at which no heartbeat has arrived for `timeout` seconds (or -1)
double trigger(const std::vector<Hb>& v, double timeout, double end)
{
    for (std::size_t i = 0; i < v.size(); ++i) {
        const double next = i + 1 < v.size() ? v[i + 1].t : end;
        if (next - v[i].t > timeout) {
            return v[i].t + timeout;
        }
    }
    return -1;
}

}  // namespace

int main(int argc, char** argv)
{
    std::ifstream in(argc > 1 ? argv[1] : "lostlink_gen.out");
    std::vector<Hb> a, b190, b1, any;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line.rfind("sec", 0) == 0) {
            continue;
        }
        std::vector<std::string> cols;
        std::istringstream ls(line);
        std::string c;
        while (std::getline(ls, c, '|')) {
            cols.push_back(c);
        }
        if (cols.size() == 4) {
            parseCell(cols[1], a);
            parseCell(cols[2], b190);
            parseCell(cols[3], b1);
        }
    }
    gaps("LOG A, vehicle 1/1 at the laptop (seq step 6)", a, 6);
    gaps("LOG B, laptop 255/190 at the vehicle (seq step 1)", b190, 1);
    gaps("LOG B, phone 255/1 at the vehicle (seq step 1)", b1, 1);
    any = b190;
    any.insert(any.end(), b1.begin(), b1.end());
    std::sort(any.begin(), any.end(), [](const Hb& x, const Hb& y) { return x.t < y.t; });
    const double t1 = trigger(any, 5.0, 50.0);
    const double t2 = trigger(b190, 5.0, 50.0);
    std::printf("failsafe, any system-255 heartbeat counts: %s\n",
                t1 < 0 ? "never triggers" : (std::to_string(t1) + " s").c_str());
    std::printf("failsafe, only the laptop 255/190 counts: triggers at %.2f s\n", t2);
    return 0;
}
