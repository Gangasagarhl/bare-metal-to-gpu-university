// uorb_lite.h - this course's model of a publish/subscribe "notice board" in the
// style of PX4's uORB: one topic = one fixed-size message type; publishers overwrite
// a small ring of the newest messages; each subscriber remembers how far it has read.
// It is our own code, written to teach the idea; it is not PX4's implementation.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace uorb_lite {

template <typename T, std::size_t QueueLen = 1>
class Topic
{
    static_assert(QueueLen >= 1, "a topic keeps at least the newest message");

public:
    explicit Topic(std::string name, int instance = 0) : name_(std::move(name)), instance_(instance) {}

    void publish(const T& msg)
    {
        std::vector<std::function<void()>> toCall;
        {
            std::lock_guard<std::mutex> lock(m_);
            ring_[generation_ % QueueLen] = msg;
            ++generation_;
            toCall = callbacks_;
        }
        for (auto& cb : toCall) {   // called outside the lock: a callback may read the topic
            cb();
        }
    }

    // Number of messages published so far (the next message gets this generation).
    std::uint64_t generation() const
    {
        std::lock_guard<std::mutex> lock(m_);
        return generation_;
    }

    // Copies message number `gen` if it is still in the ring.
    bool copy(std::uint64_t gen, T& out) const
    {
        std::lock_guard<std::mutex> lock(m_);
        if (gen >= generation_ || generation_ - gen > QueueLen) {
            return false;
        }
        out = ring_[gen % QueueLen];
        return true;
    }

    // A function to run after every publication (models "wake the subscriber's work item").
    void onPublish(std::function<void()> cb)
    {
        std::lock_guard<std::mutex> lock(m_);
        callbacks_.push_back(std::move(cb));
    }

    const std::string& name() const { return name_; }
    int instance() const { return instance_; }
    static constexpr std::size_t queueLength() { return QueueLen; }

private:
    std::string name_;
    int instance_;
    mutable std::mutex m_;
    std::array<T, QueueLen> ring_{};
    std::uint64_t generation_ = 0;
    std::vector<std::function<void()>> callbacks_;
};

template <typename T, std::size_t QueueLen = 1>
class Subscription
{
public:
    // A new subscriber starts at the newest message, if there is one (this model's choice).
    explicit Subscription(const Topic<T, QueueLen>& t) : topic_(t)
    {
        const std::uint64_t g = t.generation();
        next_ = g > 0 ? g - 1 : 0;
    }

    bool updated() const { return topic_.generation() > next_; }

    // Copies the oldest unread message that is still available; counts the ones
    // that were overwritten before this subscriber read them.
    bool update(T& out)
    {
        for (;;) {
            const std::uint64_t g = topic_.generation();
            if (g <= next_) {
                return false;
            }
            if (g - next_ > QueueLen) {
                lost_ += (g - QueueLen) - next_;
                next_ = g - QueueLen;
            }
            if (topic_.copy(next_, out)) {
                ++next_;
                return true;
            }
            // Overwritten between generation() and copy(): count it as lost and retry.
            ++lost_;
            ++next_;
        }
    }

    std::uint64_t lost() const { return lost_; }

private:
    const Topic<T, QueueLen>& topic_;
    std::uint64_t next_ = 0;
    std::uint64_t lost_ = 0;
};

} // namespace uorb_lite
