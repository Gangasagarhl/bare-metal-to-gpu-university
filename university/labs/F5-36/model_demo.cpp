// model_demo.cpp - DS401 F5-36, Listing 2: the toy verbs model in five scenes.
#include "verbs_model.hpp"

#include <cstdio>
#include <cstring>

using namespace toy;

namespace {

void bringUp(Qp& a, Qp& b)
{
    for (Qp* q : {&a, &b}) {
        q->modify(State::INIT);
        q->modify(State::RTR);
        q->modify(State::RTS);
    }
    a.connect(b);
    b.connect(a);
}

}  // namespace

int main()
{
    std::printf("toy verbs model (DS401 F5-36) - not libibverbs, no NIC involved\n");
    Node a("A", 4096);
    Node b("B", 4096);
    const Mr& aBuf = a.regMr(0, 1024, LOCAL_WRITE);
    const Mr& bLog = b.regMr(0, 1024, LOCAL_WRITE | REMOTE_WRITE);
    const Mr& bInbox = b.regMr(2048, 256, LOCAL_WRITE);

    std::printf("\n--- scene 1: the state machine ---\n");
    Qp qa(a, 0x11);
    Qp qb(b, 0x22);
    qa.modify(State::RTS);                                   // skipping states is not allowed
    qa.modify(State::INIT);
    const Wr early{1, Op::SEND, Sge{0, 8, aBuf.lkey}};
    qa.postSend(early);                                      // too early: not yet RTS
    qa.modify(State::RESET);
    bringUp(qa, qb);

    std::printf("\n--- scene 2: one-sided RDMA WRITE into B's log region ---\n");
    std::strcpy(reinterpret_cast<char*>(&a.memory[0]), "entry 1: x=42");
    qa.postSend(Wr{2, Op::RDMA_WRITE, Sge{0, 14, aBuf.lkey}, 0, bLog.rkey});
    qa.progress();
    drain(a);
    drain(b);
    std::printf("[B] memory at 0 now holds \"%s\" (B's program was not involved)\n",
                reinterpret_cast<char*>(&b.memory[0]));

    std::printf("\n--- scene 3: two-sided SEND, B posted a RECV first ---\n");
    qb.postRecv(Wr{30, Op::RECV, Sge{2048, 64, bInbox.lkey}});
    std::strcpy(reinterpret_cast<char*>(&a.memory[64]), "commit 1");
    qa.postSend(Wr{3, Op::SEND, Sge{64, 9, aBuf.lkey}});
    qa.progress();
    drain(a);
    drain(b);
    std::printf("[B] inbox holds \"%s\"\n", reinterpret_cast<char*>(&b.memory[2048]));

    std::printf("\n--- scene 4: SEND with no RECV posted at B ---\n");
    qa.postSend(Wr{4, Op::SEND, Sge{64, 9, aBuf.lkey}});
    qa.postSend(Wr{5, Op::RDMA_WRITE, Sge{0, 14, aBuf.lkey}, 0, bLog.rkey});
    qa.progress();
    drain(a);

    std::printf("\n--- scene 5: new queue pairs; RDMA WRITE past the end of B's region ---\n");
    Qp qa2(a, 0x13);
    Qp qb2(b, 0x24);
    bringUp(qa2, qb2);
    qa2.postSend(Wr{6, Op::RDMA_WRITE, Sge{0, 64, aBuf.lkey}, 1000, bLog.rkey});   // 1000 + 64 > 1024
    qa2.postSend(Wr{7, Op::RDMA_WRITE, Sge{0, 14, aBuf.lkey}, 16, bLog.rkey});
    qa2.progress();
    drain(a);
    drain(b);
    return 0;
}
