// replog_fault.cpp - DS401 F5-36, forensic evidence generator ("follower 2 never catches up").
// A leader appends log entries to two followers with one-sided RDMA WRITEs (toy verbs model).
// Each follower registers its log region and advertises (address, length, rkey, access);
// the leader prints what it was told, then its completion log. One follower was rebuilt
// with a "tightened" registration. Output is the evidence pack.
#include "verbs_model.hpp"

#include <cstdio>
#include <string>

using namespace toy;

int main()
{
    std::printf("== replicated log, leader view (toy verbs model, DS401 F5-36) ==\n");
    Node leader("Leader", 4096);
    Node f1("F1", 4096);
    Node f2("F2", 4096);
    const Mr& src = leader.regMr(0, 4096, LOCAL_WRITE);
    const Mr& log1 = f1.regMr(0, 2048, LOCAL_WRITE | REMOTE_WRITE);
    const Mr& log2 = f2.regMr(0, 2048, LOCAL_WRITE);   // build 2026-10-02: "least privilege"
    std::printf("advertised: F1 log addr %zu len %zu rkey 0x%x access 0x%x\n", log1.offset, log1.length,
                log1.rkey, log1.access);
    std::printf("advertised: F2 log addr %zu len %zu rkey 0x%x access 0x%x\n", log2.offset, log2.length,
                log2.rkey, log2.access);

    Qp l1(leader, 0x101), l2(leader, 0x102), r1(f1, 0x201), r2(f2, 0x202);
    for (Qp* q : {&l1, &l2, &r1, &r2}) {
        q->modify(State::INIT);
        q->modify(State::RTR);
        q->modify(State::RTS);
    }
    l1.connect(r1);
    r1.connect(l1);
    l2.connect(r2);
    r2.connect(l2);

    for (uint64_t e = 1; e <= 4; ++e) {
        const size_t off = (e - 1) * 64;
        const std::string entry = "term 3 index " + std::to_string(e);
        entry.copy(reinterpret_cast<char*>(&leader.memory[off]), entry.size());
        l1.postSend(Wr{100 + e, Op::RDMA_WRITE, Sge{off, 64, src.lkey}, off, log1.rkey});
        l2.postSend(Wr{200 + e, Op::RDMA_WRITE, Sge{off, 64, src.lkey}, off, log2.rkey});
    }
    l1.progress();
    l2.progress();
    int done1 = 0;
    int done2 = 0;
    Wc wc{};
    while (leader.pollCq(wc)) {
        std::printf("[Leader] completion: wr %llu %s status %s bytes %zu\n",
                    static_cast<unsigned long long>(wc.id), name(wc.op), name(wc.status), wc.bytes);
        if (wc.status == Status::SUCCESS) {
            (wc.id < 200 ? done1 : done2) += 1;
        }
    }
    std::printf("leader: entries written to F1: %d of 4; to F2: %d of 4\n", done1, done2);
    return 0;
}
