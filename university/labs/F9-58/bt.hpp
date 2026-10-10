// bt.hpp - F9-58: a minimal behaviour-tree engine (our own, about 120 lines).
// Nodes return SUCCESS, FAILURE or RUNNING each time they are ticked. Control nodes
// decide which children to tick; leaves (conditions and actions) touch the world.
#pragma once
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace bt {

enum class Status { Success, Failure, Running };

inline const char* str(Status s)
{
    return s == Status::Success ? "SUCCESS" : s == Status::Failure ? "FAILURE" : "RUNNING";
}

class Node
{
public:
    explicit Node(std::string name) : name_(std::move(name)) {}
    virtual ~Node() = default;
    virtual Status tick() = 0;
    virtual void halt() {}                 // called when a parent stops a RUNNING child
    const std::string& name() const { return name_; }
private:
    std::string name_;
};
using NodePtr = std::unique_ptr<Node>;

// Sequence: tick children in order; stop at the first that is not SUCCESS.
// reactive = true : start from the first child on every tick (conditions are re-checked);
// reactive = false: remember the running child and continue from it ("with memory").
class Sequence : public Node
{
public:
    Sequence(std::string n, bool reactive, std::vector<NodePtr> c)
        : Node(std::move(n)), reactive_(reactive), kids_(std::move(c)) {}
    Status tick() override
    {
        for (std::size_t k = reactive_ ? 0 : cur_; k < kids_.size(); ++k) {
            const Status s = kids_[k]->tick();
            if (s == Status::Running) {
                if (reactive_ && running_ != k && running_ < kids_.size()) { kids_[running_]->halt(); }
                cur_ = k;
                running_ = k;
                return s;
            }
            if (s == Status::Failure) {
                if (running_ < kids_.size() && running_ != k) { kids_[running_]->halt(); }   // stop what was running
                reset();
                return s;
            }
        }
        reset();
        return Status::Success;
    }
    void halt() override
    {
        if (running_ < kids_.size()) { kids_[running_]->halt(); }
        reset();
    }
private:
    void reset() { cur_ = 0; running_ = kids_.size(); }
    bool reactive_;
    std::vector<NodePtr> kids_;
    std::size_t cur_ = 0;
    std::size_t running_ = static_cast<std::size_t>(-1);
};

// Fallback (also called Selector): tick children in order; stop at the first that is not FAILURE.
class Fallback : public Node
{
public:
    Fallback(std::string n, std::vector<NodePtr> c) : Node(std::move(n)), kids_(std::move(c)) {}
    Status tick() override
    {
        for (std::size_t k = cur_; k < kids_.size(); ++k) {
            const Status s = kids_[k]->tick();
            if (s == Status::Running) { cur_ = k; return s; }
            if (s == Status::Success) { cur_ = 0; return s; }
        }
        cur_ = 0;
        return Status::Failure;
    }
    void halt() override
    {
        if (cur_ < kids_.size()) { kids_[cur_]->halt(); }
        cur_ = 0;
    }
private:
    std::vector<NodePtr> kids_;
    std::size_t cur_ = 0;
};

// Retry: tick the child again after a FAILURE, up to `tries` attempts in total.
class Retry : public Node
{
public:
    Retry(std::string n, int tries, NodePtr c) : Node(std::move(n)), tries_(tries), kid_(std::move(c)) {}
    Status tick() override
    {
        const Status s = kid_->tick();
        if (s == Status::Failure && ++failed_ < tries_) { return Status::Running; }   // try again next tick
        if (s != Status::Running) { failed_ = 0; }
        return s;
    }
    void halt() override { kid_->halt(); failed_ = 0; }
private:
    int tries_;
    NodePtr kid_;
    int failed_ = 0;
};

// Leaves wrap functions written for the task.
class Condition : public Node
{
public:
    Condition(std::string n, std::function<bool()> f) : Node(std::move(n)), f_(std::move(f)) {}
    Status tick() override { return f_() ? Status::Success : Status::Failure; }
private:
    std::function<bool()> f_;
};

class Action : public Node
{
public:
    // onTick(first) returns the status; `first` is true on the first tick after a start or halt.
    Action(std::string n, std::function<Status(bool)> f, std::function<void()> onHalt = nullptr)
        : Node(std::move(n)), f_(std::move(f)), onHalt_(std::move(onHalt)) {}
    Status tick() override
    {
        const Status s = f_(!active_);
        active_ = s == Status::Running;
        return s;
    }
    void halt() override
    {
        if (active_ && onHalt_) { onHalt_(); }
        active_ = false;
    }
private:
    std::function<Status(bool)> f_;
    std::function<void()> onHalt_;
    bool active_ = false;
};

}  // namespace bt
