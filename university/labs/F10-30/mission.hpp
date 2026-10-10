// mission.hpp - F10-30 Listing 5: a mission upload "microservice" on U-link. The receiving
// side (the vehicle) drives the transfer: after the count arrives it requests every item by
// its index, re-requests after a timeout, and finishes with an acknowledgement. The sending
// side (the ground station) answers requests and repeats its last message if it hears
// nothing. This is the shape of the MAVLink mission protocol as the chapter describes it;
// the timeouts and retry limits are exercise values.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "ulink.hpp"

namespace mission {

struct Item {
    std::uint16_t command;
    std::int32_t x, y;
    float z;
    std::string label;
};

using Send = std::function<void(std::vector<std::uint8_t>, const char* what)>;
inline constexpr int kTimeoutMs = 250;
inline constexpr int kMaxRetries = 5;

// D is a "dialect" bundle: D::Count, D::Request, D::ItemMsg, D::Ack (generated types).
template <class D>
class Sender {
public:
    enum class State { Idle, Sending, Done, Failed };
    Sender(std::vector<Item> items, std::uint8_t sys, std::uint8_t target, Send send)
        : items_(std::move(items)), sys_(sys), target_(target), send_(std::move(send)) {}

    void start(int now)
    {
        typename D::Count c;
        c.target_system = target_;
        c.target_component = 1;
        c.count = static_cast<std::uint16_t>(items_.size());
        last_ = ulink::encodeMsg(c, seq_++, sys_, 190);
        lastWhat_ = "COUNT " + std::to_string(items_.size());
        transmit(now);
        state = State::Sending;
    }
    void onFrame(const ulink::Frame& f, int now)
    {
        if (state != State::Sending) {
            return;
        }
        if (f.msgid == D::Request::kId) {
            const auto r = D::Request::unpack(f.payload.data(), f.payload.size());
            const Item& it = items_.at(r.seq);
            typename D::ItemMsg m;
            m.target_system = target_;
            m.target_component = 1;
            m.seq = r.seq;
            m.frame = 1;
            m.command = it.command;
            m.x = it.x;
            m.y = it.y;
            m.z = static_cast<decltype(m.z)>(it.z);
            std::memcpy(m.label.data(), it.label.data(), it.label.size() < 8 ? it.label.size() : 8);
            last_ = ulink::encodeMsg(m, seq_++, sys_, 190);
            lastWhat_ = "ITEM " + std::to_string(r.seq);
            retries_ = 0;
            transmit(now);
        } else if (f.msgid == D::Ack::kId) {
            const auto a = D::Ack::unpack(f.payload.data(), f.payload.size());
            state = a.result == 0 ? State::Done : State::Failed;
            std::printf("%6d ms  GCS  upload %s (ack result %u)\n", now,
                        state == State::Done ? "COMPLETE" : "FAILED", a.result);
        }
    }
    void onTick(int now)
    {
        if (state == State::Sending && now - lastSent_ >= kTimeoutMs) {
            if (++retries_ > kMaxRetries) {
                state = State::Failed;
                std::printf("%6d ms  GCS  upload FAILED: no answer after %d retries\n", now,
                            kMaxRetries);
                return;
            }
            std::printf("%6d ms  GCS  timeout, repeat %s (retry %d)\n", now, lastWhat_.c_str(),
                        retries_);
            transmit(now);
        }
    }
    State state = State::Idle;

private:
    void transmit(int now)
    {
        lastSent_ = now;
        send_(last_, lastWhat_.c_str());
    }
    std::vector<Item> items_;
    std::uint8_t sys_, target_;
    Send send_;
    std::vector<std::uint8_t> last_;
    std::string lastWhat_;
    std::uint8_t seq_ = 0;
    int lastSent_ = 0;
    int retries_ = 0;
};

template <class D>
class Receiver {
public:
    enum class State { Idle, Receiving, Complete, Failed };
    Receiver(std::uint8_t sys, Send send) : sys_(sys), send_(std::move(send)) {}

    void onFrame(const ulink::Frame& f, int now)
    {
        if (f.msgid == D::Count::kId && state != State::Receiving) {
            const auto c = D::Count::unpack(f.payload.data(), f.payload.size());
            expected_ = c.count;
            items.clear();
            next_ = 0;
            from_ = f.sysid;
            state = State::Receiving;
            request(now);
        } else if (f.msgid == D::ItemMsg::kId) {
            const auto m = D::ItemMsg::unpack(f.payload.data(), f.payload.size());
            if (state == State::Complete && m.seq + 1 == expected_) {
                ack(0, now);                              // our ACK was lost: send it again
            } else if (state == State::Receiving && m.seq == next_) {
                items.push_back(Item{m.command, m.x, m.y, static_cast<float>(m.z),
                                     std::string(m.label.data(), strnlen(m.label.data(), 8))});
                ++next_;
                retries_ = 0;
                if (next_ == expected_) {
                    state = State::Complete;
                    ack(0, now);
                } else {
                    request(now);
                }
            } else if (state == State::Receiving) {
                std::printf("%6d ms  VEH  item %u out of order (want %u): ignored\n", now, m.seq,
                            next_);
            }
        }
    }
    void onTick(int now)
    {
        if (state == State::Receiving && now - lastSent_ >= kTimeoutMs) {
            if (++retries_ > kMaxRetries) {
                state = State::Failed;
                std::printf("%6d ms  VEH  giving up on item %u after %d retries\n", now, next_,
                            kMaxRetries);
                ack(1, now);
                return;
            }
            std::printf("%6d ms  VEH  timeout, request item %u again (retry %d)\n", now, next_,
                        retries_);
            request(now);
        }
    }
    State state = State::Idle;
    std::vector<Item> items;

private:
    void request(int now)
    {
        typename D::Request r;
        r.target_system = from_;
        r.target_component = 190;
        r.seq = next_;
        lastSent_ = now;
        send_(ulink::encodeMsg(r, seq_++, sys_, 1), ("REQUEST " + std::to_string(next_)).c_str());
    }
    void ack(std::uint8_t result, int now)
    {
        typename D::Ack a;
        a.target_system = from_;
        a.target_component = 190;
        a.result = result;
        lastSent_ = now;
        send_(ulink::encodeMsg(a, seq_++, sys_, 1), result == 0 ? "ACK accepted" : "ACK error");
    }
    std::uint8_t sys_;
    Send send_;
    std::uint8_t seq_ = 0, from_ = 0;
    std::uint16_t expected_ = 0, next_ = 0;
    int lastSent_ = 0, retries_ = 0;
};

// A one-way link with a fixed delay that can drop chosen packets (counted from 1).
struct Pipe {
    int latencyMs = 40;
    std::vector<int> drop;
    int sent = 0;
    struct InFlight { int at; std::vector<std::uint8_t> bytes; };
    std::vector<InFlight> queue;
    bool push(std::vector<std::uint8_t> bytes, int now)
    {
        ++sent;
        for (int d : drop) {
            if (d == sent) {
                return false;
            }
        }
        queue.push_back(InFlight{now + latencyMs, std::move(bytes)});
        return true;
    }
    template <class F>
    void deliver(int now, F&& onBytes)
    {
        for (std::size_t i = 0; i < queue.size();) {
            if (queue[i].at <= now) {
                auto b = std::move(queue[i].bytes);
                queue.erase(queue.begin() + static_cast<std::ptrdiff_t>(i));
                for (std::uint8_t x : b) {
                    onBytes(x);
                }
            } else {
                ++i;
            }
        }
    }
};

}  // namespace mission
