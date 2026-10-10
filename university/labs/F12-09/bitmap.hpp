// bitmap.hpp - a frame allocator over a bitmap: one bit per 4 KiB frame, 1 = in use.
// Pure logic with no library calls, so the same header is tested on the host (with
// sanitizers) and inside the test kernel (in QEMU): "host first, then in the kernel".
#pragma once
#include <stddef.h>
#include <stdint.h>

class FrameBitmap {
public:
    static constexpr uint32_t kMaxFrames = 1024;

    explicit FrameBitmap(uint32_t frames) : frames_(frames <= kMaxFrames ? frames : kMaxFrames)
    {
        for (uint32_t& w : bits_) {
            w = 0;
        }
        free_ = frames_;
    }

    // Returns a free frame number and marks it used, or -1 if every frame is in use.
    int32_t alloc()
    {
        for (uint32_t i = 0; i < frames_; ++i) {
            if ((bits_[i / 32] & (1u << (i % 32))) == 0) {
                bits_[i / 32] |= 1u << (i % 32);
                --free_;
                return static_cast<int32_t>(i);
            }
        }
        return -1;
    }

    // Frees a used frame. Returns false (and changes nothing) for a bad or already-free frame.
    bool release(int32_t frame)
    {
        if (frame < 0 || static_cast<uint32_t>(frame) >= frames_) {
            return false;
        }
        const uint32_t i = static_cast<uint32_t>(frame);
        if ((bits_[i / 32] & (1u << (i % 32))) == 0) {
            return false;  // double free refused
        }
        bits_[i / 32] &= ~(1u << (i % 32));
        ++free_;
        return true;
    }

    uint32_t free_count() const { return free_; }
    uint32_t frames() const { return frames_; }

private:
    uint32_t bits_[kMaxFrames / 32];
    uint32_t frames_;
    uint32_t free_;
};
