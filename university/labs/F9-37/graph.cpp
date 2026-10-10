// graph.cpp - the university's model of a ROS 2-style computation graph (F9-37).
// Not ROS 2: an in-process model with nodes, topics (name + type), a service and an action.
#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

struct Twist { double v = 0.0; double w = 0.0; };            // speed, turn rate
struct Odom { double x = 0.0; double y = 0.0; double th = 0.0; };
template <typename T> struct TypeName;
template <> struct TypeName<Twist> { static constexpr const char* value = "Twist"; };
template <> struct TypeName<Odom> { static constexpr const char* value = "Odom"; };

// Name resolution in this model: "/a" absolute, "~/a" private, "a" relative to the namespace.
std::string resolve(const std::string& ns, const std::string& node, const std::string& name)
{
    if (!name.empty() && name[0] == '/') return name;
    std::string base = (ns == "/") ? "" : ns;
    if (name.rfind("~/", 0) == 0) return base + "/" + node + name.substr(1);
    return base + "/" + name;
}

struct Endpoint { std::string node; std::string topic; std::string type; bool pub; };
struct SubEntry { std::string topic; std::string type; std::function<void(const void*)> cb; };

class Graph {
public:
    std::vector<Endpoint> endpoints;
    std::vector<SubEntry> subs;
    struct Service { std::string node; std::function<bool()> handler; };
    std::map<std::string, Service> services;       // resolved name -> server node and callback
    std::deque<std::function<void()>> queue;       // callbacks waiting to run (one thread)

    template <typename T> void publish(const std::string& topic, const T& msg)
    {
        for (const auto& s : subs) {
            if (s.topic == topic && s.type == TypeName<T>::value) {
                auto copy = std::make_shared<T>(msg);          // each subscriber gets the message
                auto cb = s.cb;
                queue.push_back([cb, copy] { cb(copy.get()); });
            }
        }
    }
    // A client call: the request is queued; the response arrives later as another callback.
    bool callService(const std::string& name, std::function<void(bool)> onResponse)
    {
        auto it = services.find(name);
        if (it == services.end()) {
            std::cout << "  client: service " << name << " not available\n";
            return false;
        }
        auto handler = it->second.handler;
        queue.push_back([this, handler, onResponse] {
            bool ok = handler();                                // server callback runs
            queue.push_back([onResponse, ok] { onResponse(ok); });  // response callback queued
        });
        return true;
    }
    void spinSome()
    {
        while (!queue.empty()) {
            auto job = std::move(queue.front());
            queue.pop_front();
            job();
        }
    }
    void list() const
    {
        std::map<std::string, std::vector<const Endpoint*>> byTopic;
        for (const auto& e : endpoints) byTopic[e.topic].push_back(&e);
        std::cout << "graph: topics\n";
        for (const auto& [topic, eps] : byTopic) {
            int p = 0, s = 0;
            for (const auto* e : eps) (e->pub ? p : s)++;
            std::cout << "  " << topic << " [" << eps.front()->type << "] publishers=" << p
                      << " subscribers=" << s;
            for (const auto* e : eps) std::cout << (e->pub ? "  pub:" : "  sub:") << e->node;
            std::cout << "\n";
        }
        std::cout << "graph: services\n";
        for (const auto& [name, svc] : services) std::cout << "  " << name << " server:" << svc.node << "\n";
    }
};

class Node {
public:
    Node(Graph& g, std::string name, std::string ns) : g_(g), name_(std::move(name)), ns_(std::move(ns)) {}
    std::string fullName() const { return resolve(ns_, name_, name_); }
    template <typename T> std::string advertise(const std::string& topic)
    {
        std::string t = resolve(ns_, name_, topic);
        g_.endpoints.push_back({fullName(), t, TypeName<T>::value, true});
        return t;
    }
    template <typename T> void subscribe(const std::string& topic, std::function<void(const T&)> cb)
    {
        std::string t = resolve(ns_, name_, topic);
        g_.endpoints.push_back({fullName(), t, TypeName<T>::value, false});
        g_.subs.push_back({t, TypeName<T>::value, [cb](const void* m) { cb(*static_cast<const T*>(m)); }});
    }
    std::string offerService(const std::string& name, std::function<bool()> handler)
    {
        std::string s = resolve(ns_, name_, name);
        g_.services[s] = {fullName(), std::move(handler)};
        return s;
    }
protected:
    Graph& g_;
    std::string name_, ns_;
};

// The base node: listens to velocity commands, integrates a pose, publishes odometry.
class Base : public Node {
public:
    Base(Graph& g, const std::string& ns) : Node(g, "base", ns)
    {
        odomTopic_ = advertise<Odom>("odom");
        subscribe<Twist>("cmd_vel", [this](const Twist& t) { cmd_ = t; ++received_; });
        resetName_ = offerService("reset_odom", [this] { return resetOdom(); });
    }
    void step(double dt)   // a 10 Hz timer in a real node; called by main here
    {
        pose_.x += cmd_.v * std::cos(pose_.th) * dt;
        pose_.y += cmd_.v * std::sin(pose_.th) * dt;
        pose_.th += cmd_.w * dt;
        g_.publish(odomTopic_, pose_);
    }
    bool resetOdom() { pose_ = Odom{}; return true; }   // the service's callback
    const Odom& pose() const { return pose_; }
    int received() const { return received_; }
private:
    Odom pose_;
    Twist cmd_;
    int received_ = 0;
    std::string odomTopic_, resetName_;
};

class Teleop : public Node {
public:
    Teleop(Graph& g, const std::string& ns) : Node(g, "teleop", ns) { cmdTopic_ = advertise<Twist>("cmd_vel"); }
    void send(const Twist& t) { g_.publish(cmdTopic_, t); }
private:
    std::string cmdTopic_;
};

// An action "drive_distance": goal = metres; feedback = metres so far; result = final metres.
// Modelled as a state machine owned by a node that also reads odometry.
class Driver : public Node {
public:
    Driver(Graph& g, const std::string& ns) : Node(g, "driver", ns)
    {
        cmdTopic_ = advertise<Twist>("cmd_vel");
        subscribe<Odom>("odom", [this](const Odom& o) { onOdom(o); });
    }
    void sendGoal(double metres)
    {
        goal_ = metres; active_ = true; started_ = false;
        std::printf("  action drive_distance: goal %.2f m accepted\n", metres);
    }
    void cancel() { if (active_) { active_ = false; stop(); std::printf("  action drive_distance: canceled at %.2f m\n", done_); } }
    bool active() const { return active_; }
private:
    void onOdom(const Odom& o)
    {
        if (!active_) return;
        if (!started_) { start_ = o; started_ = true; }
        done_ = std::hypot(o.x - start_.x, o.y - start_.y);
        if (done_ >= goal_ - 1e-9) {
            active_ = false; stop();
            std::printf("  action drive_distance: result SUCCEEDED, travelled %.2f m\n", done_);
            return;
        }
        std::printf("  action drive_distance: feedback %.2f m\n", done_);
        g_.publish(cmdTopic_, Twist{0.5, 0.0});
    }
    void stop() { g_.publish(cmdTopic_, Twist{}); }
    std::string cmdTopic_;
    Odom start_;
    double goal_ = 0.0, done_ = 0.0;
    bool active_ = false, started_ = false;
};

int main()
{
    std::string key, teleopNs = "/", baseNs = "/", driverNs = "/";
    while (std::cin >> key) {
        if (key == "teleop_ns") std::cin >> teleopNs;
        else if (key == "base_ns") std::cin >> baseNs;
        else if (key == "driver_ns") std::cin >> driverNs;
    }
    Graph g;
    Base base(g, baseNs);
    Teleop teleop(g, teleopNs);
    Driver driver(g, driverNs);
    g.list();

    const double dt = 0.1;
    std::cout << "phase 1: teleop sends v=0.5 m/s, w=0.0 for 1.0 s\n";
    for (int i = 0; i < 10; ++i) { teleop.send(Twist{0.5, 0.0}); g.spinSome(); base.step(dt); g.spinSome(); }
    teleop.send(Twist{}); g.spinSome();
    std::printf("  base received %d commands; pose x=%.2f y=%.2f th=%.2f\n",
                base.received(), base.pose().x, base.pose().y, base.pose().th);

    std::cout << "phase 2: service call reset_odom (from teleop's namespace)\n";
    bool sent = g.callService(resolve(teleopNs, "teleop", "reset_odom"), [&base](bool ok) {
        std::printf("  client: response success=%s; pose x=%.2f\n", ok ? "true" : "false", base.pose().x);
    });
    if (sent) std::printf("  client: request sent, queue holds %zu callback(s)\n", g.queue.size());
    g.spinSome();

    std::cout << "phase 3: action drive_distance 0.30 m\n";
    driver.sendGoal(0.30);
    for (int i = 0; i < 20 && driver.active(); ++i) { base.step(dt); g.spinSome(); }
    base.step(dt); g.spinSome();
    std::printf("  final pose x=%.2f (commands received in total %d)\n", base.pose().x, base.received());
    return 0;
}
