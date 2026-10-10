// Two short Paxos stories: one proposer alone; then a second proposer that
// arrives after a value was chosen and must adopt it.
#include "paxos.h"

int main()
{
    std::printf("--- story 1: proposer P1 alone, value A ---\n");
    paxos::World w1(1);
    w1.propose(11, "A");
    w1.runUntil(50);
    w1.report();

    std::printf("\n--- story 2: P1 proposes A; later P2 proposes B ---\n");
    paxos::World w2(1);
    w2.setDelay(11, 3, 40);  // P1's messages to acceptor 3 are slow
    w2.propose(11, "A");
    w2.runUntil(30);
    w2.propose(12, "B");
    w2.runUntil(120);
    w2.report();
    return 0;
}
