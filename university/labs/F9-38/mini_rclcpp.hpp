// mini_rclcpp.hpp - the university's teaching model of an rclcpp-shaped C++ API (F9-38).
// NOT rclcpp. It copies the SHAPE of a ROS 2 C++ program (a Node class, shared pointers to
// publishers, subscriptions and timers, callbacks, spin) so the ideas can be built and run
// here. Simulated time; one thread; messages delivered in-process.
#pragma once
#include <algorithm>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mini {

struct Context;
Context& context();

struct SubscriptionBase {
    std::string topic;
    std::string node;
    virtual ~SubscriptionBase() = default;
    virtual bool takeAndRun() = 0;          // run the callback for one queued message
};

struct TimerBase {
    long periodMs = 0;
    long nextMs = 0;
    std::function<void()> cb;
};

struct Context {
    long nowMs = 0;
    std::vector<std::weak_ptr<SubscriptionBase>> subs;   // weak: the node owns them
    std::vector<std::weak_ptr<TimerBase>> timers;        // weak: the node owns them
    int liveSubscribers(const std::string& topic)
    {
        int n = 0;
        for (auto& w : subs) {
            if (auto s = w.lock(); s && s->topic == topic) ++n;
        }
        return n;
    }
};

inline Context& context()
{
    static Context c;
    return c;
}

template <typename T>
struct Subscription : SubscriptionBase {
    std::deque<T> queue;
    std::size_t depth = 10;
    std::function<void(const T&)> cb;
    bool takeAndRun() override
    {
        if (queue.empty()) return false;
        T msg = std::move(queue.front());
        queue.pop_front();
        cb(msg);
        return true;
    }
};

template <typename T>
class Publisher {
public:
    explicit Publisher(std::string topic) : topic_(std::move(topic)) {}
    void publish(const T& msg)
    {
        for (auto& w : context().subs) {
            auto s = std::dynamic_pointer_cast<Subscription<T>>(w.lock());
            if (!s || s->topic != topic_) continue;
            s->queue.push_back(msg);
            if (s->queue.size() > s->depth) s->queue.pop_front();   // keep the newest 'depth'
        }
    }
    int get_subscription_count() const { return context().liveSubscribers(topic_); }
private:
    std::string topic_;
};

class Node {
public:
    explicit Node(std::string name) : name_(std::move(name)) {}
    virtual ~Node() = default;
    const std::string& get_name() const { return name_; }

    template <typename T>
    std::shared_ptr<Publisher<T>> create_publisher(const std::string& topic, std::size_t /*depth*/)
    {
        return std::make_shared<Publisher<T>>(topic);
    }
    template <typename T>
    std::shared_ptr<Subscription<T>> create_subscription(const std::string& topic, std::size_t depth,
                                                         std::function<void(const T&)> cb)
    {
        auto s = std::make_shared<Subscription<T>>();
        s->topic = topic; s->node = name_; s->depth = depth; s->cb = std::move(cb);
        context().subs.push_back(s);           // the context keeps only a weak reference
        return s;
    }
    std::shared_ptr<TimerBase> create_wall_timer(long periodMs, std::function<void()> cb)
    {
        auto t = std::make_shared<TimerBase>();
        t->periodMs = periodMs; t->nextMs = context().nowMs + periodMs; t->cb = std::move(cb);
        context().timers.push_back(t);
        return t;
    }
    void log(const char* level, const std::string& text) const
    {
        std::printf("[%s] [t=%.3f] [%s]: %s\n", level, context().nowMs / 1000.0, name_.c_str(), text.c_str());
    }
private:
    std::string name_;
};

// spin for a stretch of simulated time: fire due timers, then run every queued message callback.
inline void spin_for(long durationMs)
{
    Context& c = context();
    long end = c.nowMs + durationMs;
    while (true) {
        long next = end + 1;
        for (auto& w : c.timers) {
            if (auto t = w.lock()) next = std::min(next, t->nextMs);
        }
        if (next > end) break;
        c.nowMs = next;
        for (auto& w : c.timers) {
            auto t = w.lock();
            if (t && t->nextMs == next) { t->nextMs += t->periodMs; t->cb(); }
        }
        bool ran = true;
        while (ran) {
            ran = false;
            for (auto& w : c.subs) {
                if (auto s = w.lock()) ran = s->takeAndRun() || ran;
            }
        }
    }
    c.nowMs = end;
}

}  // namespace mini
