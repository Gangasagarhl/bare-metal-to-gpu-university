// F6-28 Listing 2: a timeline model of streams, events and engines.
// It is a MODEL, not a GPU: it applies the documented ordering rules (work in one
// stream runs in issue order; work in different streams may overlap; an event wait
// orders one stream behind another; the legacy default stream 0 synchronises with
// the other blocking streams) to engines that the input describes.
// Model assumptions (not vendor facts): each engine unit runs one operation at a
// time, in the order the host issued work to it; one kernel runs at a time.
//
// Input, one command per line ('#' starts a comment):
//   scenario <name>                    start a new scenario
//   copyengines <1|2>                  1: both copy directions share one engine
//   stream <id> blocking|nonblocking   (stream 0 is the legacy default stream)
//   h2d|kernel|d2h <stream> <duration> <label> [hostsync]
//   record <event> <stream>            event captures the stream's work so far
//   wait <stream> <event>              later work in <stream> waits for the event
//   host <duration> <label>            the host thread computes (no GPU work)
//   sync <stream>                      the host waits until <stream> is idle
//   csv                                also print the timeline as CSV lines
//   end                                schedule and print the scenario
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Op
{
    std::string lane;    // engine or host lane the op occupies
    std::string label;
    int stream = -1;     // -1 for host work
    double start = 0.0;
    double end = 0.0;
};

struct Scenario
{
    std::string name;
    int copyEngines = 2;
    bool csv = false;
    std::map<int, bool> nonBlocking;          // stream id -> created non-blocking?
    std::map<int, double> streamTail;         // end time of the last op in each stream
    std::map<int, double> streamWaitUntil;    // pending event waits per stream
    std::map<std::string, double> events;     // event -> completion time it captured
    std::map<std::string, double> laneFree;   // engine lane -> time it becomes free
    double hostClock = 0.0;
    double legacyTail = 0.0;                  // end of the last op issued to stream 0
    double blockingTail = 0.0;                // end of the last op in any blocking stream
    std::vector<Op> ops;
};

std::string laneFor(const Scenario& s, const std::string& kind)
{
    if (kind == "kernel") { return "compute"; }
    if (s.copyEngines == 1) { return "copy"; }
    return kind == "h2d" ? "copy-in" : "copy-out";
}

void issue(Scenario& s, const std::string& kind, int stream, double dur, const std::string& label,
           bool hostSync)
{
    const bool isLegacy = (stream == 0);
    const bool blocking = isLegacy || !s.nonBlocking[stream];
    double ready = std::max(s.hostClock, s.streamTail[stream]);   // issue time, stream order
    ready = std::max(ready, s.streamWaitUntil[stream]);            // event waits
    if (isLegacy) { ready = std::max(ready, s.blockingTail); }     // waits for blocking streams
    if (blocking) { ready = std::max(ready, s.legacyTail); }       // blocking streams wait for 0
    const std::string lane = laneFor(s, kind);
    const double start = std::max(ready, s.laneFree[lane]);
    const double end = start + dur;
    s.laneFree[lane] = end;
    s.streamTail[stream] = end;
    if (isLegacy) { s.legacyTail = end; }
    if (blocking) { s.blockingTail = std::max(s.blockingTail, end); }
    if (hostSync) { s.hostClock = end; }                           // the call blocks the host
    s.ops.push_back({lane, label, stream, start, end});
}

void drawLane(const std::string& lane, const std::vector<Op>& ops, double total)
{
    const int width = 64;
    std::string row(width, '.');
    for (const Op& o : ops) {
        if (o.lane != lane || o.end <= o.start) { continue; }
        const int a = static_cast<int>(o.start / total * width);
        const int b = std::max(a + 1, static_cast<int>(o.end / total * width));
        const char mark = o.stream < 0 ? 'H' : static_cast<char>('0' + o.stream % 10);
        for (int x = a; x < b && x < width; ++x) { row[static_cast<std::size_t>(x)] = mark; }
    }
    std::printf("  %-9s|%s|\n", lane.c_str(), row.c_str());
}

double busy(const std::string& lane, const std::vector<Op>& ops)
{
    double t = 0.0;
    for (const Op& o : ops) {
        if (o.lane == lane) { t += o.end - o.start; }
    }
    return t;
}

// Time during which at least one copy and one kernel run at the same moment.
double copyComputeOverlap(const std::vector<Op>& ops)
{
    std::vector<double> cuts;
    for (const Op& o : ops) { cuts.push_back(o.start); cuts.push_back(o.end); }
    std::sort(cuts.begin(), cuts.end());
    double overlap = 0.0;
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
        const double mid = 0.5 * (cuts[i] + cuts[i + 1]);
        bool copy = false;
        bool kernel = false;
        for (const Op& o : ops) {
            if (o.start <= mid && mid < o.end) {
                if (o.lane == "compute") { kernel = true; }
                if (o.lane.rfind("copy", 0) == 0) { copy = true; }
            }
        }
        if (copy && kernel) { overlap += cuts[i + 1] - cuts[i]; }
    }
    return overlap;
}

void finish(const Scenario& s)
{
    double total = 0.0;
    for (const Op& o : s.ops) { total = std::max(total, o.end); }
    std::printf("scenario %s  (copy engines: %d)\n", s.name.c_str(), s.copyEngines);
    for (const Op& o : s.ops) {
        std::printf("  %-9s stream %-2s %-16s %7.1f -> %7.1f\n", o.lane.c_str(),
                    o.stream < 0 ? "-" : std::to_string(o.stream).c_str(), o.label.c_str(),
                    o.start, o.end);
    }
    std::vector<std::string> lanes;
    for (const Op& o : s.ops) {
        if (std::find(lanes.begin(), lanes.end(), o.lane) == lanes.end()) {
            lanes.push_back(o.lane);
        }
    }
    std::sort(lanes.begin(), lanes.end());
    std::printf("  timeline (digit = stream id, H = host work), total %.1f\n", total);
    for (const std::string& l : lanes) { drawLane(l, s.ops, total); }
    for (const std::string& l : lanes) {
        std::printf("  busy %-9s %7.1f (%5.1f %%)\n", l.c_str(), busy(l, s.ops),
                    total > 0 ? 100.0 * busy(l, s.ops) / total : 0.0);
    }
    std::printf("  copy/compute overlap %.1f; total %.1f\n", copyComputeOverlap(s.ops), total);
    if (s.csv) {
        std::printf("  csv: lane,start,end,stream,label\n");
        for (const Op& o : s.ops) {
            std::printf("  csv: %s,%.1f,%.1f,%d,%s\n", o.lane.c_str(), o.start, o.end, o.stream,
                        o.label.c_str());
        }
    }
    std::printf("\n");
}

int main()
{
    Scenario s;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (const auto hash = line.find('#'); hash != std::string::npos) { line.erase(hash); }
        std::istringstream in(line);
        std::string cmd;
        if (!(in >> cmd)) { continue; }
        if (cmd == "scenario") {
            s = Scenario{};
            in >> s.name;
        } else if (cmd == "copyengines") {
            in >> s.copyEngines;
        } else if (cmd == "stream") {
            int id = 0;
            std::string kind;
            in >> id >> kind;
            s.nonBlocking[id] = (kind == "nonblocking");
        } else if (cmd == "h2d" || cmd == "kernel" || cmd == "d2h") {
            int stream = 0;
            double dur = 0.0;
            std::string label, flag;
            in >> stream >> dur >> label >> flag;
            issue(s, cmd, stream, dur, label, flag == "hostsync");
        } else if (cmd == "record") {
            std::string ev;
            int stream = 0;
            in >> ev >> stream;
            s.events[ev] = std::max(s.streamTail[stream], s.streamWaitUntil[stream]);
        } else if (cmd == "wait") {
            int stream = 0;
            std::string ev;
            in >> stream >> ev;
            s.streamWaitUntil[stream] = std::max(s.streamWaitUntil[stream], s.events[ev]);
        } else if (cmd == "host") {
            double dur = 0.0;
            std::string label;
            in >> dur >> label;
            s.ops.push_back({"host", label, -1, s.hostClock, s.hostClock + dur});
            s.hostClock += dur;
        } else if (cmd == "sync") {
            int stream = 0;
            in >> stream;
            s.hostClock = std::max(s.hostClock, s.streamTail[stream]);
        } else if (cmd == "csv") {
            s.csv = true;
        } else if (cmd == "end") {
            finish(s);
        } else {
            std::printf("unknown command: %s\n", cmd.c_str());
            return 1;
        }
    }
    return 0;
}
