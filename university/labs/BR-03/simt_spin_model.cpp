// BR-03 Listing 5: a MODEL (plain C++, not a GPU) of one group of lanes running two lock
// programs under two schedulers. It shows why the CPU spin lock of F2-38 can hang a warp.
//   Program A (CPU habit): spin: if CAS(lock, 0 -> 1) fails, goto spin   | reconverge | counter++; unlock
//   Program B (warp-safe): loop: if CAS succeeds { counter++; unlock; done } if !done goto loop | reconverge
//   Scheduler "lock-step": one program counter for the group; lanes that leave the loop wait at the
//     reconvergence point until every lane has left it (the model of SIMT without independent scheduling).
//   Scheduler "independent": one program counter per lane, one step per lane in turn (like OS threads,
//     or like a GPU with independent thread scheduling, which this model does not claim to reproduce).
#include <cstdio>
#include <vector>

struct Result
{
    bool finished = false;
    int counter = 0;
    long rounds = 0;
    long failedCas = 0;
    int waitingAtReconvergence = 0;
    int stillSpinning = 0;
};

constexpr long kRoundLimit = 10'000;

Result lockStep(int width, bool programB)
{
    Result r;
    int lock = 0;
    std::vector<bool> inLoop(static_cast<std::size_t>(width), true);
    int left = width;
    while (left > 0 && r.rounds < kRoundLimit) {
        ++r.rounds;                                         // one trip round the loop for the group
        int winner = -1;
        for (int l = 0; l < width; ++l) {                   // the CAS instruction: all active lanes,
            if (!inLoop[static_cast<std::size_t>(l)]) { continue; }  // same address, one after another
            if (lock == 0) { lock = 1; winner = l; } else { ++r.failedCas; }
        }
        if (winner >= 0) {                                  // the next instructions, winner only
            if (programB) { ++r.counter; lock = 0; }        // B: work and unlock inside the loop
            inLoop[static_cast<std::size_t>(winner)] = false;
            --left;
        }
    }
    if (left > 0) {                                         // nobody can reach the unlock
        r.waitingAtReconvergence = width - left;
        r.stillSpinning = left;
        return r;
    }
    if (!programB) { r.counter = width; }                   // A: work after reconvergence
    r.finished = true;
    return r;
}

Result independent(int width, bool programB)
{
    Result r;
    int lock = 0;
    enum Pc { Spin, Work, Unlock, Done };
    std::vector<Pc> pc(static_cast<std::size_t>(width), Spin);
    int done = 0;
    while (done < width && r.rounds < kRoundLimit) {
        ++r.rounds;
        for (int l = 0; l < width; ++l) {                   // every lane gets one step per round
            Pc& p = pc[static_cast<std::size_t>(l)];
            if (p == Spin) {
                if (lock == 0) { lock = 1; p = Work; } else { ++r.failedCas; }
            } else if (p == Work) {
                ++r.counter;
                p = programB ? Done : Unlock;
                if (programB) { lock = 0; ++done; }         // B releases in the same step
            } else if (p == Unlock) {
                lock = 0;
                p = Done;
                ++done;
            }
        }
    }
    r.finished = (done == width);
    return r;
}

void print(const char* program, const char* scheduler, int width, const Result& r)
{
    if (r.finished) {
        std::printf("%-22s %-12s width %2d: finished, counter = %2d, %4ld rounds, %5ld failed CAS\n",
                    program, scheduler, width, r.counter, r.rounds, r.failedCas);
    } else {
        std::printf("%-22s %-12s width %2d: NO PROGRESS after %ld rounds: %d lane(s) wait at the "
                    "reconvergence point holding the lock, %d lane(s) still spinning\n",
                    program, scheduler, width, r.rounds, r.waitingAtReconvergence, r.stillSpinning);
    }
}

int main()
{
    for (int width : {32, 64}) {
        print("A: CPU spin lock", "lock-step", width, lockStep(width, false));
        print("A: CPU spin lock", "independent", width, independent(width, false));
        print("B: work inside loop", "lock-step", width, lockStep(width, true));
        print("B: work inside loop", "independent", width, independent(width, true));
    }
    return 0;
}
