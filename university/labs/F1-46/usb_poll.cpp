// F1-46 forensic evidence: "the button that sometimes does nothing".
// A small USB device (a made-up foot pedal) reports the pedal's CURRENT state
// (pressed or not) whenever the host asks; it keeps no history. The host asks
// once every `interval` ms. The user taps the pedal; each tap is held for a
// few ms. A tap the host never sees while it is held is lost.
// Exercise numbers in ms, not a real product.
#include <cstdio>
#include <vector>

struct Tap
{
    int start;   // ms
    int length;  // ms held down
};

int seen_taps(const std::vector<Tap>& taps, int interval, bool print)
{
    int seen = 0;
    for (const Tap& t : taps) {
        bool hit = false;
        // polls happen at 0, interval, 2*interval, ...
        const int first_poll = ((t.start + interval - 1) / interval) * interval;
        if (first_poll < t.start + t.length) {
            hit = true;
        }
        seen += hit ? 1 : 0;
        if (print) {
            std::printf("  tap at %4d ms held %2d ms  next poll at %4d ms  -> %s\n", t.start,
                        t.length, first_poll, hit ? "reported" : "MISSED");
        }
    }
    return seen;
}

int main()
{
    std::vector<Tap> taps;
    const int lengths[] = {4, 9, 6, 12, 5, 3, 15, 7, 4, 10};
    for (int i = 0; i < 10; ++i) {
        taps.push_back({i * 97 + 13, lengths[i]});
    }
    std::printf("device log: 10 taps; host polls the pedal's interrupt IN endpoint\n");
    std::printf("shipped firmware, polling interval 16 ms:\n");
    const int a = seen_taps(taps, 16, true);
    std::printf("reported %d of 10\n", a);
    for (int interval : {1, 2, 4, 8, 16, 32}) {
        std::printf("interval %2d ms: reported %2d of 10\n", interval, seen_taps(taps, interval, false));
    }
    return 0;
}
