// mission.hpp - F9-58: the fetch-a-cup mission on a small simulated world, as a behaviour tree.
// World: places joined by routes of known length; the robot moves 0.2 m per tick (0.4 m/s,
// ticks every 0.5 s). The cup slips out of the gripper on the first grasp attempt.
#pragma once
#include "bt.hpp"
#include <cstdio>
#include <string>

struct World
{
    std::string at = "living room";   // last place reached ("" while moving)
    std::string target;               // where the robot is driving to
    double remaining = 0.0;           // metres left on the current route
    bool doorOpen = false;            // the kitchen door is closed today
    bool holding = false;
    bool cupOnSofaTable = false;
    double battery = 60.0;            // per cent
    int graspAttempts = 0;
    int tick = 0;

    void send(const std::string& to, double length) { target = to; remaining = length; at = ""; }
    void step()                       // called once per tick, after the tree
    {
        if (!target.empty()) {
            remaining -= 0.2;
            battery -= 0.25;
            if (remaining <= 1e-9) { at = target; target.clear(); remaining = 0.0; }
        }
        battery -= 0.05;
        ++tick;
    }
    std::string where() const { return at.empty() ? "driving to " + target : "at " + at; }
};

inline void log(const World& w, const std::string& msg)
{
    std::printf("tick %3d  t=%5.1fs  battery %4.1f%%  %-28s %s\n", w.tick, 0.5 * w.tick, w.battery, w.where().c_str(),
                msg.c_str());
}

// GoTo: asynchronous. First tick sends the goal; RUNNING until the robot arrives.
// fireAndForget = true reproduces the forensic fault: SUCCESS as soon as the goal is sent.
inline bt::NodePtr goTo(World& w, const std::string& place, double length, bool fireAndForget)
{
    return std::make_unique<bt::Action>(
        "GoTo(" + place + ")",
        [&w, place, length, fireAndForget](bool first) {
            if (first) {
                w.send(place, length);
                log(w, "GoTo(" + place + ") started, route " + std::to_string(length).substr(0, 3) + " m");
                if (fireAndForget) { return bt::Status::Success; }
            }
            if (w.at == place) { log(w, "GoTo(" + place + ") SUCCESS"); return bt::Status::Success; }
            return bt::Status::Running;
        },
        [&w, place]() {
            log(w, "GoTo(" + place + ") HALTED: robot stops");
            w.target.clear();
            w.at = "corridor";
        });
}

inline bt::NodePtr makeTree(World& w, bool fireAndForget)
{
    using namespace bt;
    std::vector<NodePtr> viaDoor;
    viaDoor.push_back(std::make_unique<Condition>("door open?", [&w] { return w.doorOpen; }));
    viaDoor.push_back(goTo(w, "kitchen table", 4.6, fireAndForget));
    std::vector<NodePtr> reach;
    reach.push_back(std::make_unique<Sequence>("via the door", false, std::move(viaDoor)));
    reach.push_back(goTo(w, "kitchen table", 9.2, fireAndForget));        // the long way, via the hall
    auto grasp = std::make_unique<Action>("Grasp(cup)", [&w](bool first) {
        if (first) { log(w, "Grasp(cup) started"); return Status::Running; }   // closing takes one tick
        ++w.graspAttempts;
        if (w.at != "kitchen table") { log(w, "Grasp(cup) FAILURE: no cup within reach"); return Status::Failure; }
        if (w.graspAttempts == 1) { log(w, "Grasp(cup) FAILURE: cup slipped"); return Status::Failure; }
        w.holding = true;
        log(w, "Grasp(cup) SUCCESS");
        return Status::Success;
    });
    auto place = std::make_unique<Action>("Place(cup)", [&w](bool) {
        if (!w.holding || w.at != "sofa table") { log(w, "Place(cup) FAILURE: not holding a cup at the sofa table"); return Status::Failure; }
        w.holding = false;
        w.cupOnSofaTable = true;
        log(w, "Place(cup) SUCCESS");
        return Status::Success;
    });
    std::vector<NodePtr> fetch;
    fetch.push_back(std::make_unique<Fallback>("reach the kitchen", std::move(reach)));
    fetch.push_back(std::make_unique<Retry>("retry grasp (3 tries)", 3, std::move(grasp)));
    fetch.push_back(goTo(w, "sofa table", 5.0, fireAndForget));
    fetch.push_back(std::move(place));
    std::vector<NodePtr> mission;
    mission.push_back(std::make_unique<Condition>("battery above 20%?", [&w] { return w.battery > 20.0; }));
    mission.push_back(std::make_unique<Sequence>("fetch", false, std::move(fetch)));
    std::vector<NodePtr> root;
    root.push_back(std::make_unique<Sequence>("mission (reactive)", true, std::move(mission)));
    root.push_back(goTo(w, "dock", 3.0, fireAndForget));
    return std::make_unique<Fallback>("root", std::move(root));
}

// Tick the tree every 0.5 s until it returns SUCCESS or FAILURE (or 200 ticks pass).
inline bt::Status runMission(World& w, bool fireAndForget)
{
    bt::NodePtr tree = makeTree(w, fireAndForget);
    bt::Status s = bt::Status::Running;
    while (s == bt::Status::Running && w.tick < 200) {
        s = tree->tick();
        w.step();
    }
    log(w, std::string("tree returned ") + bt::str(s) + (w.cupOnSofaTable ? "; cup is on the sofa table" : "; cup NOT delivered"));
    return s;
}
