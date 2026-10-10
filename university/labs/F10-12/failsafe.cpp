// F10-12 Listing 2: how a flight controller decides that the RC link is lost.
// Frames arrive every 20 ms; the transmitter goes silent from t = 1000 ms to 1600 ms.
// Three receiver behaviours during the silence are compared. The 20 ms frame period and
// the 300 ms timeout are exercise values; real ones are settings documented by the
// receiver's maker and the flight stack.
#include <cstdio>

enum class Rx { StopsOutput, SendsFailsafeFlag, HoldsLastValues };

struct Detector
{
    int timeoutMs = 300;
    int lastGoodMs = 0;
    bool lost = false;
    int failsafes = 0;

    // called every millisecond; frame is true when a frame arrived in this millisecond
    void step(int t, bool frame, bool failsafeFlag)
    {
        if (frame && !failsafeFlag) {
            lastGoodMs = t;
            if (lost) {
                lost = false;
                std::printf("    %5d ms  link regained\n", t);
            }
        }
        if (!lost && (t - lastGoodMs > timeoutMs || (frame && failsafeFlag))) {
            lost = true;
            ++failsafes;
            std::printf("    %5d ms  FAILSAFE: link lost (last good frame at %d ms)\n", t,
                        lastGoodMs);
        }
    }
};

void run(const char* title, Rx rx)
{
    std::printf("%s\n", title);
    Detector d;
    for (int t = 0; t <= 2500; ++t) {
        const bool tick = (t % 20 == 0);
        const bool silent = (t >= 1000 && t < 1600);
        bool frame = tick, flag = false;
        if (silent) {
            frame = tick && rx != Rx::StopsOutput;
            flag = rx == Rx::SendsFailsafeFlag;
        }
        d.step(t, frame, flag);
    }
    if (d.failsafes == 0) {
        std::printf("    (no event: the controller never noticed the 600 ms silence)\n");
    }
}

int main()
{
    run("A: receiver stops sending frames", Rx::StopsOutput);
    run("B: receiver keeps sending frames with its failsafe flag set", Rx::SendsFailsafeFlag);
    run("C: receiver keeps sending the last stick values, no flag", Rx::HoldsLastValues);
    return 0;
}
