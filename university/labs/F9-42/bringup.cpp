// bringup.cpp - the university's model of parameters and a lifecycle (managed) node (F9-42).
// Not ROS 2. Parameters are declared with a default, a type and a validity rule; overrides come
// from a parameter file (stdin); a launcher-like main() drives the node through its lifecycle.
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <numbers>
#include <string>
#include <vector>

enum class State { Unconfigured, Inactive, Active, Finalized };
const char* name(State s)
{
    switch (s) {
    case State::Unconfigured: return "unconfigured";
    case State::Inactive: return "inactive";
    case State::Active: return "active";
    default: return "finalized";
    }
}

struct Param { double value; std::function<bool(double)> valid; std::string rule; bool integer; };

class OdomNode {
public:
    explicit OdomNode(const std::map<std::string, double>& overrides)
    {
        declare("wheel_radius", 0.10, [](double v) { return v > 0.0; }, "> 0 m", false);
        declare("ticks_per_rev", 1024, [](double v) { return v >= 1.0; }, ">= 1", true);
        declare("publish_rate_hz", 20, [](double v) { return v >= 1.0 && v <= 200.0; }, "1..200", true);
        for (const auto& [key, v] : overrides) {               // keys look like "odom.wheel_radius"
            auto dot = key.find('.');
            if (key.substr(0, dot) != "odom") continue;
            std::string p = key.substr(dot + 1);
            if (!params_.count(p)) { std::printf("[odom] warning: override '%s' ignored: not a declared parameter\n", p.c_str()); continue; }
            if (!set(p, v)) std::printf("[odom] override '%s' rejected\n", p.c_str());
        }
    }
    bool set(const std::string& p, double v)
    {
        auto it = params_.find(p);
        if (it == params_.end()) { std::printf("[odom] set %s: no such parameter\n", p.c_str()); return false; }
        Param& par = it->second;
        if (par.integer && v != std::floor(v)) { std::printf("[odom] set %s=%g rejected: integer expected\n", p.c_str(), v); return false; }
        if (!par.valid(v)) { std::printf("[odom] set %s=%g rejected: must be %s\n", p.c_str(), v, par.rule.c_str()); return false; }
        par.value = v;
        std::printf("[odom] parameter %s = %g\n", p.c_str(), v);
        return true;
    }
    // Lifecycle transitions: allowed only from the right state (the table); a callback may fail.
    bool transition(const std::string& t)
    {
        struct Edge { const char* name; State from; State to; };
        static const Edge edges[] = {
            {"configure", State::Unconfigured, State::Inactive}, {"cleanup", State::Inactive, State::Unconfigured},
            {"activate", State::Inactive, State::Active},        {"deactivate", State::Active, State::Inactive},
            {"shutdown", State::Unconfigured, State::Finalized}, {"shutdown", State::Inactive, State::Finalized},
            {"shutdown", State::Active, State::Finalized}};
        for (const auto& e : edges) {
            if (t != e.name || state_ != e.from) continue;
            bool ok = true;
            if (t == "configure") ok = onConfigure();      // a callback may refuse
            if (t == "cleanup") ticksTotal_ = 0;
            std::printf("[odom] %s: %s -> %s\n", t.c_str(), name(state_), name(ok ? e.to : state_));
            if (ok) state_ = e.to;
            return ok;
        }
        std::printf("[odom] transition '%s' not allowed in state %s\n", t.c_str(), name(state_));
        return false;
    }
    void encoderTicks(long ticks)          // a subscription callback in a real node
    {
        if (state_ != State::Active) { std::printf("[odom] not active: %ld ticks ignored\n", ticks); return; }
        ticksTotal_ += ticks;
        double metres = static_cast<double>(ticksTotal_) / perRev_ * 2.0 * std::numbers::pi * radius_;
        std::printf("[odom] publish odom: distance %.3f m (ticks %ld)\n", metres, ticksTotal_);
    }
private:
    void declare(const std::string& p, double def, std::function<bool(double)> valid, const std::string& rule, bool integer)
    {
        params_[p] = {def, std::move(valid), rule, integer};
    }
    bool onConfigure()                     // read parameters once, allocate, check
    {
        radius_ = params_["wheel_radius"].value;
        perRev_ = params_["ticks_per_rev"].value;
        std::printf("[odom] on_configure: wheel_radius=%g ticks_per_rev=%g publish_rate_hz=%g\n", radius_, perRev_,
                    params_["publish_rate_hz"].value);
        return true;
    }
    std::map<std::string, Param> params_;
    State state_ = State::Unconfigured;
    double radius_ = 0.0, perRev_ = 1.0;
    long ticksTotal_ = 0;
};

int main()
{
    // stdin: "param <node>.<name> <value>" lines (the parameter file), then "drive <ticks>" lines.
    std::map<std::string, double> overrides;
    std::vector<long> drives;
    std::string word;
    while (std::cin >> word) {
        if (word == "param") { std::string k; double v; std::cin >> k >> v; overrides[k] = v; }
        else if (word == "drive") { long t; std::cin >> t; drives.push_back(t); }
    }
    std::printf("launch: starting node odom with %zu parameter override(s)\n", overrides.size());
    OdomNode odom(overrides);
    odom.transition("activate");                     // wrong order: refused
    odom.encoderTicks(100);                          // not active yet: nothing published
    odom.transition("configure");
    odom.transition("activate");
    for (long t : drives) odom.encoderTicks(t);
    odom.set("publish_rate_hz", 500);                // at run time: rejected by the rule
    odom.set("wheel_radius", -0.05);                 // rejected
    odom.transition("deactivate");
    odom.transition("cleanup");
    odom.transition("shutdown");
    return 0;
}
