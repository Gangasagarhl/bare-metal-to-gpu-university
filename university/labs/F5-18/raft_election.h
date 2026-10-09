// raft_election.h - leader election (Raft paper, section 5.2; Figure 2 RequestVote).
#pragma once
#include "raft.h"

namespace raft {

// Follower or candidate whose election timer ran out.
inline void Node::startElection()
{
    role_ = Role::Candidate;
    st_.currentTerm += 1;    // a new term ...
    st_.votedFor = id_;      // ... in which I vote for myself
    persist();               // before anyone hears about it
    votes_ = {id_};
    leaderId_ = 0;
    resetElectionTimer();    // if this election fails, try again later
    note("election timeout -> candidate, asks for votes (last entry %d, term %d)",
         lastIndex(), termAt(lastIndex()));
    for (int p : peers()) {
        Message m;
        m.type = MsgType::RequestVote;
        m.to = p;
        m.lastLogIndex = lastIndex();
        m.lastLogTerm = termAt(lastIndex());
        send(m);
    }
    if (quorum(votes_)) {    // a cluster of one
        becomeLeader();
    }
}

// Election restriction (section 5.4.1): the candidate's log must be at least as
// up-to-date as mine: later last term wins; same last term, longer log wins.
inline bool Node::candidateIsUpToDate(int lastTerm, int lastIdx) const
{
    const int myTerm = termAt(lastIndex());
    return lastTerm > myTerm || (lastTerm == myTerm && lastIdx >= lastIndex());
}

inline void Node::handleRequestVote(const Message& m)
{
    // receive() has already adopted m.term if it was newer.
    bool grant = false;
    const char* why = "";
    if (m.term < st_.currentTerm) {
        why = "its term is old";
    } else if (st_.votedFor != 0 && st_.votedFor != m.from) {
        why = "already voted in this term";
    } else if (!candidateIsUpToDate(m.lastLogTerm, m.lastLogIndex)) {
        why = "its log is behind mine";
    } else {
        grant = true;
        st_.votedFor = m.from;
        resetElectionTimer();  // granting a vote counts as hearing from a candidate
    }
    persist();  // the vote must be on disk before the reply leaves
    if (grant) {
        note("votes for n%d", m.from);
    } else {
        note("refuses vote to n%d (%s)", m.from, why);
    }
    Message r;
    r.type = MsgType::VoteReply;
    r.to = m.from;
    r.ok = grant;
    send(r);
}

inline void Node::handleVoteReply(const Message& m)
{
    if (role_ != Role::Candidate || m.term != st_.currentTerm || !m.ok) {
        return;  // stale reply, or a refusal
    }
    votes_.insert(m.from);
    if (quorum(votes_)) {
        becomeLeader();
    }
}

inline void Node::becomeLeader()
{
    role_ = Role::Leader;
    leaderId_ = id_;
    nextIndex_.clear();
    matchIndex_.clear();
    for (int p : peers()) {
        nextIndex_[p] = lastIndex() + 1;  // optimistic: assume followers match
        matchIndex_[p] = 0;               // but know nothing for sure yet
    }
    std::string got;
    for (int v : votes_) {
        got += " n" + std::to_string(v);
    }
    note("WINS election with votes from%s", got.c_str());
    env_.becameLeader(id_, st_.currentTerm);
    if (opt_.noopOnElection) {
        Entry e;
        e.type = OpType::Noop;
        appendAsLeader(e);  // also sends the first heartbeat
    } else {
        broadcastAppendEntries();
    }
}

} // namespace raft
