// fetch_fsm.cpp - F9-58: the same mission as an explicit finite-state machine,
// on the same World, for comparison with fetch_bt.cpp.
#include "mission.hpp"

enum class S { Start, GoKitchenDoor, GoKitchenHall, Grasp, GoSofa, Place, GoDock, Done, Failed };

const char* name(S s)
{
    const char* n[] = {"Start", "GoKitchenDoor", "GoKitchenHall", "Grasp", "GoSofa", "Place", "GoDock", "Done", "Failed"};
    return n[static_cast<int>(s)];
}

S runFsm(World& w)
{
    S s = S::Start;
    bool entered = false;                         // false right after a transition
    int transitions = 0;
    auto go = [&](S next, const std::string& why) {
        log(w, std::string(name(s)) + " -> " + name(next) + " (" + why + ")");
        s = next;
        entered = false;
        ++transitions;
    };
    while (s != S::Done && s != S::Failed && w.tick < 200) {
        const bool moving = s == S::GoKitchenDoor || s == S::GoKitchenHall || s == S::GoSofa;
        if (moving && w.battery <= 20.0) {        // this check must be repeated for every state it guards
            w.target.clear();
            w.at = "corridor";
            go(S::GoDock, "battery low");
        }
        const bool first = !entered;
        entered = true;
        switch (s) {
        case S::Start: go(w.doorOpen ? S::GoKitchenDoor : S::GoKitchenHall, w.doorOpen ? "door open" : "door closed"); break;
        case S::GoKitchenDoor:
        case S::GoKitchenHall:
            if (first) { w.send("kitchen table", s == S::GoKitchenDoor ? 4.6 : 9.2); }
            else if (w.at == "kitchen table") { go(S::Grasp, "arrived"); }
            break;
        case S::Grasp:
            if (first) { break; }                 // closing the gripper takes one tick
            ++w.graspAttempts;
            if (w.at == "kitchen table" && w.graspAttempts > 1) { w.holding = true; go(S::GoSofa, "grasped"); }
            else if (w.graspAttempts >= 3) { go(S::GoDock, "grasp failed 3 times"); }
            else { entered = false; log(w, "Grasp: cup slipped, trying again"); }
            break;
        case S::GoSofa:
            if (first) { w.send("sofa table", 5.0); }
            else if (w.at == "sofa table") { go(S::Place, "arrived"); }
            break;
        case S::Place:
            w.holding = false;
            w.cupOnSofaTable = true;
            go(S::Done, "placed");
            break;
        case S::GoDock:
            if (first) { w.send("dock", 3.0); }
            else if (w.at == "dock") { go(S::Failed, "docked, mission not completed"); }
            break;
        default: break;
        }
        w.step();
    }
    log(w, std::string("state machine ended in ") + name(s) + " after " + std::to_string(transitions) + " transitions");
    return s;
}

int main()
{
    std::printf("run 1: battery 60%%, kitchen door closed\n");
    World w1;
    runFsm(w1);
    std::printf("\nrun 2: battery 26%%, kitchen door closed\n");
    World w2;
    w2.battery = 26.0;
    runFsm(w2);
    return 0;
}
