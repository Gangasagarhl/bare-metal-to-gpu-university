// router.cpp - F10-31 Listing 1: how a node with several links decides where a message
// goes. It learns on which link each (system, component) was heard, forwards a message that
// has no target (or target system 0) to every other link, and forwards a targeted message
// only to the link where its target lives. These are the university's routing rules,
// modelled on the routing behaviour the MAVLink Developer Guide describes (see the
// chapter's unverified box for the exact rules).
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct Msg {
    int link;              // where it arrived
    int sys, comp;         // sender
    std::string name;
    int targetSys;         // -1: the message has no target fields; 0: broadcast
    int targetComp;
};

class Router {
public:
    Router(int sys, int comp, std::vector<std::string> links)
        : sys_(sys), comp_(comp), links_(std::move(links)) {}

    void handle(const Msg& m)
    {
        routes_[{m.sys, m.comp}] = m.link;            // learn where the sender lives
        std::vector<int> out;
        if (m.targetSys <= 0) {
            for (int l = 0; l < static_cast<int>(links_.size()); ++l) {
                if (l != m.link) { out.push_back(l); }
            }
        } else {
            for (const auto& [key, link] : routes_) {
                const bool sysMatch = key.first == m.targetSys;
                const bool compMatch = m.targetComp == 0 || key.second == m.targetComp;
                if (sysMatch && compMatch && link != m.link) {
                    bool dup = false;
                    for (int o : out) { dup = dup || o == link; }
                    if (!dup) { out.push_back(link); }
                }
            }
        }
        const bool forMe = m.targetSys <= 0 ||
                           (m.targetSys == sys_ && (m.targetComp == 0 || m.targetComp == comp_));
        std::printf("%-16s from %3d/%-3d on %-6s -> %s", m.name.c_str(), m.sys, m.comp,
                    links_[static_cast<std::size_t>(m.link)].c_str(), forMe ? "[used here] " : "");
        if (out.empty() && forMe) {
            std::printf("\n");
        } else if (out.empty()) {
            std::printf("(nowhere: target %d/%d not heard yet)\n", m.targetSys, m.targetComp);
        } else {
            for (int o : out) { std::printf("%s ", links_[static_cast<std::size_t>(o)].c_str()); }
            std::printf("\n");
        }
    }

private:
    int sys_, comp_;
    std::vector<std::string> links_;
    std::map<std::pair<int, int>, int> routes_;
};

int main()
{
    // The flight controller (1/1) routes between its three links.
    Router fc(1, 1, {"USB", "TELEM1", "TELEM2"});
    fc.handle({1, 255, 190, "U_HEARTBEAT", -1, 0});    // laptop GCS on the telemetry radio
    fc.handle({2, 1, 191, "U_HEARTBEAT", -1, 0});      // companion computer on TELEM2
    fc.handle({1, 255, 190, "U_COMMAND", 1, 191});     // GCS asks the companion to start a camera
    fc.handle({2, 1, 191, "U_COMMAND_ACK", 255, 190}); // the companion answers the GCS
    fc.handle({2, 1, 191, "U_MISSION_COUNT", 1, 1});   // for the autopilot itself: not forwarded
    fc.handle({1, 255, 190, "U_COMMAND", 1, 0});       // to every component of system 1
    fc.handle({0, 254, 1, "U_COMMAND", 7, 1});         // unknown target system 7
    return 0;
}
