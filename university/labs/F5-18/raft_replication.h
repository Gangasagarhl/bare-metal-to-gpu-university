// raft_replication.h - log replication and commitment (Raft paper, sections
// 5.3 and 5.4.2; Figure 2 AppendEntries and the "Rules for Servers").
#pragma once
#include "raft.h"

namespace raft {

inline void Node::appendAsLeader(Entry e)
{
    e.term = st_.currentTerm;
    st_.log.push_back(e);
    refreshConfig();
    persist();
    note("appends entry %d: %s", lastIndex(), describe(e).c_str());
    advanceCommitIndex();  // matters only when the leader alone is a majority
    broadcastAppendEntries();
}

inline void Node::sendAppendEntries(int peer)
{
    if (nextIndex_.count(peer) == 0) {  // a server that just joined the configuration
        nextIndex_[peer] = lastIndex() + 1;
        matchIndex_[peer] = 0;
    }
    const int next = nextIndex_[peer];
    Message m;
    m.type = MsgType::AppendEntries;
    m.to = peer;
    m.prevLogIndex = next - 1;
    m.prevLogTerm = termAt(next - 1);
    m.leaderCommit = commitIndex_;
    for (int i = next; i <= lastIndex() && i < next + opt_.maxBatch; ++i) {
        m.entries.push_back(st_.log[i - 1]);
    }
    send(m);
}

inline void Node::broadcastAppendEntries()
{
    for (int p : peers()) {
        sendAppendEntries(p);
    }
    nextHeartbeat_ = env_.now() + opt_.heartbeat;
}

inline void Node::handleAppendEntries(const Message& m)
{
    Message r;
    r.type = MsgType::AppendReply;
    r.to = m.from;
    if (m.term < st_.currentTerm) {  // a deposed leader: tell it the newer term
        r.ok = false;
        r.lastLogIndex = lastIndex();
        send(r);
        return;
    }
    if (role_ == Role::Leader) {
        note("ERROR: AppendEntries from n%d in my own term: two leaders!", m.from);
        return;
    }
    if (role_ == Role::Candidate) {
        note("hears from leader n%d of this term", m.from);
    }
    role_ = Role::Follower;
    leaderId_ = m.from;
    resetElectionTimer();

    int index = m.prevLogIndex;
    if (faults_.skipPrevLogCheck) {
        index = std::min(m.prevLogIndex, lastIndex());  // FAULT: "close enough"
    } else if (m.prevLogIndex > lastIndex() || termAt(m.prevLogIndex) != m.prevLogTerm) {
        // Consistency check failed: I do not have the leader's entry before these.
        note("rejects AppendEntries: no entry %d with term %d (my last entry is %d)",
             m.prevLogIndex, m.prevLogTerm, lastIndex());
        r.ok = false;
        r.lastLogIndex = lastIndex();
        send(r);
        return;
    }
    int added = 0;
    for (const Entry& e : m.entries) {
        ++index;
        if (index <= lastIndex() && termAt(index) != e.term) {
            const int gone = lastIndex() - index + 1;
            note("conflict at entry %d (have %s, leader has %s): deletes %d entr%s",
                 index, describe(st_.log[index - 1]).c_str(), describe(e).c_str(), gone,
                 gone == 1 ? "y" : "ies");
            st_.log.resize(index - 1);
        }
        if (index > lastIndex()) {
            st_.log.push_back(e);
            ++added;
        }
    }
    if (added == 1) {
        note("stores entry %d from n%d", index, m.from);
    } else if (added > 1) {
        note("stores entries %d-%d from n%d", index - added + 1, index, m.from);
    }
    refreshConfig();
    persist();
    if (m.leaderCommit > commitIndex_) {
        commitIndex_ = std::max(commitIndex_, std::min(m.leaderCommit, index));
        applyCommitted();
    }
    r.ok = true;
    r.matchIndex = index;
    r.lastLogIndex = lastIndex();
    send(r);
}

inline void Node::handleAppendReply(const Message& m)
{
    if (role_ != Role::Leader || m.term != st_.currentTerm) {
        return;
    }
    if (m.ok) {
        matchIndex_[m.from] = std::max(matchIndex_[m.from], m.matchIndex);
        nextIndex_[m.from] = matchIndex_[m.from] + 1;
        advanceCommitIndex();
        if (nextIndex_[m.from] <= lastIndex()) {
            sendAppendEntries(m.from);  // more to send
        }
    } else {
        // Step back and retry; jump straight to the follower's end if it is shorter.
        nextIndex_[m.from] = std::max(1, std::min(nextIndex_[m.from] - 1, m.lastLogIndex + 1));
        sendAppendEntries(m.from);
    }
}

// "If there exists an N such that N > commitIndex, a majority of matchIndex[i] >= N,
//  and log[N].term == currentTerm: set commitIndex = N" (Figure 2, leaders).
inline void Node::advanceCommitIndex()
{
    if (role_ != Role::Leader) {
        return;
    }
    for (int n = lastIndex(); n > commitIndex_; --n) {
        if (termAt(n) != st_.currentTerm && !faults_.commitOldTermByCount) {
            continue;  // never commit an earlier term's entry by counting replicas
        }
        std::set<int> have = {id_};
        for (const auto& [peer, match] : matchIndex_) {
            if (match >= n) {
                have.insert(peer);
            }
        }
        if (quorum(have)) {
            note("commits up to entry %d (%s)", n, describe(st_.log[n - 1]).c_str());
            commitIndex_ = n;
            applyCommitted();
            membershipStep();
            return;
        }
    }
}

inline void Node::handleClientRequest(const Message& m)
{
    Message r;
    r.type = MsgType::ClientReply;
    r.to = m.from;
    r.op = m.op;
    if (role_ != Role::Leader) {
        r.ok = false;
        r.leaderHint = leaderId_;
        send(r);
        return;
    }
    if (m.op.type == OpType::Get && faults_.readFromLocalState) {
        r.ok = true;  // FAULT: answered without asking a majority
        r.result = kv_.count(m.op.key) ? kv_.at(m.op.key) : 0;
        send(r);
        return;
    }
    Entry e = m.op;
    e.client = m.from;
    if (e.type == OpType::Config) {
        const Config& c = config();
        bool pending = c.joint();
        for (int i = lastIndex(); i > commitIndex_; --i) {
            pending = pending || st_.log[i - 1].type == OpType::Config;
        }
        if (pending) {
            note("refuses a configuration change: another one is in progress");
            r.ok = false;
            r.leaderHint = id_;
            send(r);
            return;
        }
        e.config.joining = m.op.config.members;  // C_old,new
        e.config.members = c.members;
    }
    appendAsLeader(e);
}

} // namespace raft
