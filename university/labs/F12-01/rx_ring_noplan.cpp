// rx_ring_noplan.cpp - SE301 F12-01 forensic evidence generator.
// The same receive ring as it was written WITHOUT a plan: indices kept modulo 8,
// no "full" test, no drop counter. Each input line is one paste into the serial
// console while the console task is busy; the task reads only after the paste.
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

class RxRingNoPlan
{
public:
    static constexpr std::size_t kSize = 8;

    void push(std::uint8_t b)                       // interrupt handler
    {
        buf_[head_] = b;
        head_ = (head_ + 1) % kSize;
    }

    bool pop(std::uint8_t& b)                       // console task
    {
        if (head_ == tail_) {
            return false;                           // "empty"
        }
        b = buf_[tail_];
        tail_ = (tail_ + 1) % kSize;
        return true;
    }

    std::size_t dropped() const { return 0; }       // nothing ever counts a loss

private:
    std::array<std::uint8_t, kSize> buf_{};
    std::size_t head_ = 0;
    std::size_t tail_ = 0;
};

int main()
{
    RxRingNoPlan ring;
    std::string paste;
    int n = 0;
    while (std::getline(std::cin, paste)) {
        for (char c : paste) {
            ring.push(static_cast<std::uint8_t>(c));
        }
        std::string got;
        std::uint8_t b = 0;
        while (ring.pop(b)) {
            got += static_cast<char>(b);
        }
        std::printf("paste %d: sent %2zu bytes \"%s\"\n", ++n, paste.size(), paste.c_str());
        std::printf("         console got %2zu bytes \"%s\"   dropped counter: %zu\n",
                    got.size(), got.c_str(), ring.dropped());
    }
    return 0;
}
