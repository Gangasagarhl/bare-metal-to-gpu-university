// ring_model.h - F1-52: a NIC receive descriptor ring in the university's own simplified
// model. The driver owns the slots from tail to head; the NIC owns the slots from head
// up to (not including) tail. A descriptor here has a buffer, a length and a "done" flag;
// the real descriptor layout of a given NIC is defined in that NIC's datasheet.
#pragma once
#include <cstdio>
#include <string>
#include <vector>

struct RxDescriptor
{
    int buffer = -1;       // which packet buffer the driver attached (-1: none)
    int length = 0;        // written by the NIC
    bool done = false;     // written by the NIC: "this slot holds a received packet"
};

class ToyNic
{
public:
    explicit ToyNic(int slots) : ring_(static_cast<std::size_t>(slots)) {}

    std::vector<RxDescriptor>& ring() { return ring_; }
    int head() const { return head_; }
    int tail() const { return tail_; }
    long missed() const { return missed_; }

    // The driver writes the tail register: "slots up to here have fresh buffers".
    void writeTail(int t) { tail_ = t; }

    // A packet arrives from the wire. The NIC needs a slot it owns (head != tail).
    bool receive(int length)
    {
        if (head_ == tail_) {
            ++missed_;     // no free descriptor: the packet is dropped
            return false;
        }
        RxDescriptor& d = ring_[static_cast<std::size_t>(head_)];
        d.length = length;  // (the real NIC copies the bytes into d.buffer by DMA here)
        d.done = true;
        head_ = (head_ + 1) % static_cast<int>(ring_.size());
        return true;
    }

private:
    std::vector<RxDescriptor> ring_;
    int head_ = 0;
    int tail_ = 0;
    long missed_ = 0;
};

// Prints the ring as one line: owner of each slot and its state.
inline void showRing(ToyNic& nic)
{
    std::string s;
    const int n = static_cast<int>(nic.ring().size());
    for (int i = 0; i < n; ++i) {
        const RxDescriptor& d = nic.ring()[static_cast<std::size_t>(i)];
        const bool nicOwns = (nic.head() <= nic.tail())
                                 ? (i >= nic.head() && i < nic.tail())
                                 : (i >= nic.head() || i < nic.tail());
        s += nicOwns ? " N" : " D";
        s += d.done ? "*" : (d.buffer >= 0 ? "+" : ".");
    }
    std::printf("  ring:%s   head=%d tail=%d missed=%ld\n", s.c_str(), nic.head(), nic.tail(), nic.missed());
}
