// two_values.h - the forensic story of F5-17: one slow Accept message, and an
// acceptor rule. bug = true: the acceptor compares an Accept with the last ballot
// it ACCEPTED; bug = false: with the last ballot it PROMISED (the correct rule).
#pragma once
#include "paxos.h"

inline int twoValuesStory(bool bug)
{
    paxos::World w(1, bug);
    w.setRetry(200, 0);       // no retries during this short story
    w.setDelay(12, 1, 100);   // P2 cannot reach a1 quickly
    w.propose(11, "A");
    w.runUntil(9);
    w.setDelay(11, 2, 40);    // from now on P1's messages to a2 and a3 are slow
    w.setDelay(11, 3, 40);
    w.runUntil(20);
    w.propose(12, "B");
    w.runUntil(21);
    w.setDelay(12, 2, 60);    // P2's Accept to a2 will be slow
    w.runUntil(150);
    return w.report() ? 0 : 1;
}
