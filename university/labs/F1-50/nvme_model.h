// nvme_model.h - F1-50: one submission queue (SQ) and one completion queue (CQ) with
// doorbells and a phase tag, in the university's own simplified model (not a driver).
// Entries hold only a command id (and, in the CQ, the SQ head and the phase bit); the real
// entry layouts are defined by the NVM Express specifications.
#pragma once
#include <cstdio>
#include <vector>

struct Completion
{
    int cid = 0;
    int sqHead = 0;   // where the controller has consumed the SQ up to
    int phase = 0;    // memory starts zeroed, so phase 0 means "not written in pass 1"
};

class Controller
{
public:
    explicit Controller(int n) : sq_(static_cast<std::size_t>(n)), cq_(static_cast<std::size_t>(n)) {}
    std::vector<int>& sqMemory() { return sq_; }
    const std::vector<Completion>& cqMemory() const { return cq_; }

    // The host wrote the SQ tail doorbell: run every command between head and tail.
    void sqTailDoorbell(int newTail)
    {
        const int n = static_cast<int>(sq_.size());
        while (sqHead_ != newTail) {
            const int cid = sq_[static_cast<std::size_t>(sqHead_)];
            sqHead_ = (sqHead_ + 1) % n;
            cq_[static_cast<std::size_t>(cqTail_)] = Completion{cid, sqHead_, phase_};
            std::printf("  device: ran cid %d, wrote CQ slot %d with phase %d\n", cid, cqTail_, phase_);
            cqTail_ = (cqTail_ + 1) % n;
            if (cqTail_ == 0) {
                phase_ ^= 1;   // a new pass over the CQ: invert the phase it writes
            }
        }
    }

private:
    std::vector<int> sq_;
    std::vector<Completion> cq_;
    int sqHead_ = 0;
    int cqTail_ = 0;
    int phase_ = 1;
};

class Host
{
public:
    // flipPhaseOnWrap = false reproduces the bug of the forensic lab.
    Host(Controller& c, int n, bool flipPhaseOnWrap = true)
        : c_(c), n_(n), flip_(flipPhaseOnWrap) {}

    bool submit(int cid)
    {
        if ((sqTail_ + 1) % n_ == sqHead_) {
            std::printf("  host: SQ full, cid %d must wait\n", cid);
            return false;
        }
        c_.sqMemory()[static_cast<std::size_t>(sqTail_)] = cid;
        std::printf("  host: put cid %d in SQ slot %d\n", cid, sqTail_);
        sqTail_ = (sqTail_ + 1) % n_;
        return true;
    }

    void ring()
    {
        std::printf("  host: write SQ tail doorbell = %d\n", sqTail_);
        c_.sqTailDoorbell(sqTail_);
    }

    // Returns how many completions were consumed.
    int poll()
    {
        int got = 0;
        for (;;) {
            const Completion& e = c_.cqMemory()[static_cast<std::size_t>(cqHead_)];
            if (e.phase != expectedPhase_) {
                std::printf("  host: CQ slot %d has phase %d, expected %d -> nothing new\n",
                            cqHead_, e.phase, expectedPhase_);
                break;
            }
            std::printf("  host: CQ slot %d: cid %d done (phase %d), SQ head now %d\n",
                        cqHead_, e.cid, e.phase, e.sqHead);
            sqHead_ = e.sqHead;
            ++got;
            cqHead_ = (cqHead_ + 1) % n_;
            if (cqHead_ == 0 && flip_) {
                expectedPhase_ ^= 1;   // a new pass: now expect the inverted phase
            }
        }
        std::printf("  host: write CQ head doorbell = %d\n", cqHead_);
        return got;
    }

private:
    Controller& c_;
    int n_;
    int sqTail_ = 0;
    int sqHead_ = 0;
    int cqHead_ = 0;
    int expectedPhase_ = 1;
    bool flip_;
};
