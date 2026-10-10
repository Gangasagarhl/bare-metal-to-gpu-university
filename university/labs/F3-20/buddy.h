// buddy.h - F3-20: a binary buddy allocator for physical frames.
// A block of order k is 2^k frames, aligned to 2^k frames. Free blocks are kept on one
// doubly linked list per order; the list nodes live inside the free frames themselves,
// reached through a "window" (the kernel's direct map; a plain array in the host test).
// One state byte per frame records whether the frame heads a free block, and of which order.
// Host-tested in pmm_host.cpp.
#pragma once
#include <cstdint>

class Buddy {
public:
    static constexpr int kMaxOrder = 10;              // largest block: 2^10 frames = 4 MiB
    static constexpr uint8_t kNotFreeHead = 0xFF;
    static constexpr uint8_t kAllocated = 0x80;       // | order, when order checking is on

    struct Stats {
        uint64_t free_frames;
        uint64_t allocs;
        uint64_t frees;
        uint64_t free_blocks[kMaxOrder + 1];
    };

    // window: virtual address of physical address 0. state: one byte per frame.
    void init(uint8_t* window, uint8_t* state, uint64_t nframes, bool check_order)
    {
        window_ = window;
        state_ = state;
        nframes_ = nframes;
        check_order_ = check_order;
        for (uint64_t f = 0; f < nframes; ++f) {
            state_[f] = kNotFreeHead;
        }
        for (auto& h : heads_) {
            h = nullptr;
        }
        stats_ = Stats{};
    }

    // Hands the frames [first, first + count) to the allocator, as the largest aligned blocks.
    void add_frames(uint64_t first, uint64_t count)
    {
        uint64_t f = first;
        uint64_t end = first + count;
        while (f < end) {
            int order = kMaxOrder;
            while (order > 0 && ((f & ((1ull << order) - 1)) != 0 || f + (1ull << order) > end)) {
                --order;
            }
            if (check_order_) {
                state_[f] = static_cast<uint8_t>(kAllocated | order);
            }
            release(f, order);
            f += 1ull << order;
        }
        stats_.frees = 0;
    }

    // Allocates 2^order contiguous frames; returns the first frame number in 'frame'.
    bool alloc(int order, uint64_t& frame)
    {
        int o = order;
        while (o <= kMaxOrder && heads_[o] == nullptr) {
            ++o;
        }
        if (o > kMaxOrder) {
            return false;
        }
        uint64_t f = pop(o);
        while (o > order) {                   // split: keep the lower half, free the upper
            --o;
            push(f + (1ull << o), o);
        }
        state_[f] = check_order_ ? static_cast<uint8_t>(kAllocated | order) : kNotFreeHead;
        ++stats_.allocs;
        frame = f;
        return true;
    }

    // Frees a block. Returns false (and changes nothing) on a detected error:
    // double free, or (with order checking) an order different from the one allocated.
    bool free(uint64_t frame, int order)
    {
        if (frame >= nframes_ || order < 0 || order > kMaxOrder) {
            return false;
        }
        if (check_order_ ? state_[frame] != (kAllocated | order) : state_[frame] != kNotFreeHead) {
            return false;                     // a free block's head, or a wrong order
        }
        release(frame, order);
        ++stats_.frees;
        return true;
    }

    const Stats& stats() const { return stats_; }
    uint8_t state(uint64_t frame) const { return state_[frame]; }

private:
    struct Node {
        Node* next;
        Node* prev;
    };

    Node* node(uint64_t frame) { return reinterpret_cast<Node*>(window_ + frame * 4096); }
    uint64_t frame_of(const Node* n) const
    {
        return static_cast<uint64_t>(reinterpret_cast<const uint8_t*>(n) - window_) / 4096;
    }

    void push(uint64_t f, int order)
    {
        Node* n = node(f);
        n->prev = nullptr;
        n->next = heads_[order];
        if (n->next != nullptr) {
            n->next->prev = n;
        }
        heads_[order] = n;
        state_[f] = static_cast<uint8_t>(order);
        stats_.free_frames += 1ull << order;
        ++stats_.free_blocks[order];
    }

    void unlink(uint64_t f, int order)
    {
        Node* n = node(f);
        if (n->prev != nullptr) {
            n->prev->next = n->next;
        } else {
            heads_[order] = n->next;
        }
        if (n->next != nullptr) {
            n->next->prev = n->prev;
        }
        state_[f] = kNotFreeHead;
        stats_.free_frames -= 1ull << order;
        --stats_.free_blocks[order];
    }

    uint64_t pop(int order)
    {
        uint64_t f = frame_of(heads_[order]);
        unlink(f, order);
        return f;
    }

    void release(uint64_t f, int order)       // free with merging ("coalescing")
    {
        state_[f] = kNotFreeHead;
        while (order < kMaxOrder) {
            uint64_t buddy = f ^ (1ull << order);
            if (buddy >= nframes_ || state_[buddy] != order) {
                break;
            }
            unlink(buddy, order);
            f &= ~(1ull << order);            // the merged block starts at the lower buddy
            ++order;
        }
        push(f, order);
    }

    uint8_t* window_ = nullptr;
    uint8_t* state_ = nullptr;
    uint64_t nframes_ = 0;
    bool check_order_ = false;
    Node* heads_[kMaxOrder + 1] = {};
    Stats stats_{};
};
