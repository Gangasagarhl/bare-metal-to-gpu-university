// raft.h - one Raft server for the DS302 simulator: types, state and the rules
// shared by every role. Elections are in raft_election.h, log replication in
// raft_replication.h, configuration changes in raft_membership.h.
// Rules follow Figure 2 and sections 5-6 of the Raft paper (extended version).
#pragma once
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "sim.h"

namespace raft {

using sim::Time;

enum class Role { Follower, Candidate, Leader };

inline const char* roleName(Role r)
{
    switch (r) {
    case Role::Follower: return "follower";
    case Role::Candidate: return "candidate";
    case Role::Leader: return "LEADER";
    }
    return "?";
}

enum class OpType { Noop, Put, Get, Config };

struct Config
{
    std::vector<int> members;  // the configuration (C_old during a change)
    std::vector<int> joining;  // C_new while in joint consensus; empty otherwise
    bool joint() const { return !joining.empty(); }
};

struct Entry
{
    int term = 0;
    OpType type = OpType::Noop;
    char key = 'x';
    int value = 0;
    int client = 0;   // client process that asked (0: the server itself)
    int request = 0;  // that client's request number
    Config config;    // only for OpType::Config
};

inline std::string listIds(const std::vector<int>& ids)
{
    std::string s = "{";
    for (std::size_t i = 0; i < ids.size(); ++i) {
        s += (i ? "," : "") + std::to_string(ids[i]);
    }
    return s + "}";
}

inline std::string describe(const Entry& e)
{
    std::string s = "t" + std::to_string(e.term) + " ";
    switch (e.type) {
    case OpType::Noop: return s + "no-op";
    case OpType::Put: return s + "put " + e.key + "=" + std::to_string(e.value);
    case OpType::Get: return s + "get " + e.key;
    case OpType::Config:
        return s + "config " + listIds(e.config.members) +
               (e.config.joint() ? "+" + listIds(e.config.joining) : "");
    }
    return s;
}

inline bool sameEntry(const Entry& a, const Entry& b)
{
    return a.term == b.term && a.type == b.type && a.key == b.key && a.value == b.value &&
           a.client == b.client && a.request == b.request &&
           a.config.members == b.config.members && a.config.joining == b.config.joining;
}

enum class MsgType { RequestVote, VoteReply, AppendEntries, AppendReply, ClientRequest, ClientReply };

struct Message
{
    MsgType type{};
    int from = 0;
    int to = 0;
    int term = 0;
    int lastLogIndex = 0;  // RequestVote: candidate's last entry; AppendReply: follower's
    int lastLogTerm = 0;
    bool ok = false;       // VoteReply: vote granted; AppendReply: success; ClientReply: done
    int prevLogIndex = 0;  // AppendEntries
    int prevLogTerm = 0;
    int leaderCommit = 0;
    std::vector<Entry> entries;
    int matchIndex = 0;    // AppendReply: last index known to match the leader
    Entry op;              // ClientRequest / ClientReply
    int leaderHint = 0;    // ClientReply when not leader
    int result = 0;        // ClientReply for a get
};

struct Options
{
    Time electionMin = 150;      // randomized election timeout range (ms)
    Time electionMax = 300;
    Time heartbeat = 50;         // leader sends AppendEntries at least this often
    int maxBatch = 8;            // entries per AppendEntries
    bool noopOnElection = true;  // new leader appends a no-op entry of its term
    std::map<int, Time> fixedTimeout;  // scripted scenarios: node -> fixed timeout
};

// Deliberate bugs for the forensic labs. All false means: Raft as the paper says.
struct Faults
{
    bool lazyPersist = false;          // write state to disk only every 100 ms
    bool skipPrevLogCheck = false;     // follower appends without the consistency check
    bool commitOldTermByCount = false; // count replicas of entries from earlier terms
    bool readFromLocalState = false;   // leader answers get from its own state machine
};

// What a server may use from the world around it (the Cluster implements this).
class Env
{
public:
    virtual ~Env() = default;
    virtual Time now() const = 0;
    virtual void send(const Message& m) = 0;
    virtual sim::Rng& rng() = 0;
    virtual void trace(int node, int term, Role role, const std::string& text) = 0;
    virtual void becameLeader(int node, int term) = 0;
    virtual void applied(int node, int index, const Entry& e) = 0;
};

// The state a server must keep on stable storage (Figure 2, "Persistent state").
struct Persistent
{
    int currentTerm = 0;
    int votedFor = 0;  // 0 = voted for nobody in currentTerm (ids start at 1)
    std::vector<Entry> log;  // log[0] holds index 1
};

class Node
{
public:
    Node(int id, Config boot, Options opt, Faults faults, Env& env)
        : id_(id), boot_(std::move(boot)), cfg_(boot_), opt_(std::move(opt)), faults_(faults),
          env_(env)
    {
        resetElectionTimer();
    }

    int id() const { return id_; }
    bool up() const { return up_; }
    Role role() const { return role_; }
    int term() const { return st_.currentTerm; }
    int votedFor() const { return st_.votedFor; }
    int commitIndex() const { return commitIndex_; }
    const std::vector<Entry>& log() const { return st_.log; }
    const std::map<char, int>& state() const { return kv_; }
    int lastIndex() const { return static_cast<int>(st_.log.size()); }
    int termAt(int index) const { return index == 0 ? 0 : st_.log[index - 1].term; }
    const Config& config() const { return cfg_; }

    void tick();                       // called every simulated millisecond
    void receive(const Message& m);    // a message arrived
    void crash() { up_ = false; }      // memory is lost; disk_ survives
    void restart();

private:
    // shared rules (this file)
    void persist();
    void becomeFollower(int term);
    void resetElectionTimer();
    void applyCommitted();
    void send(Message m);
    void note(const char* fmt, ...) __attribute__((format(printf, 2, 3)));
    // raft_election.h
    void startElection();
    bool candidateIsUpToDate(int lastTerm, int lastIndex) const;
    void handleRequestVote(const Message& m);
    void handleVoteReply(const Message& m);
    void becomeLeader();
    // raft_replication.h
    void appendAsLeader(Entry e);
    void sendAppendEntries(int peer);
    void broadcastAppendEntries();
    void handleAppendEntries(const Message& m);
    void handleAppendReply(const Message& m);
    void advanceCommitIndex();
    void handleClientRequest(const Message& m);
    // raft_membership.h
    void refreshConfig();
    bool isMember(int id) const;
    std::vector<int> peers() const;
    bool quorum(const std::set<int>& ids) const;
    void membershipStep();

    int id_;
    Config boot_;  // configuration before any configuration entry is in the log
    Config cfg_;   // latest configuration in the log (or boot_); see refreshConfig()
    Options opt_;
    Faults faults_;
    Env& env_;

    Persistent st_;    // in memory
    Persistent disk_;  // what survives a crash
    bool dirty_ = false;

    bool up_ = true;
    Role role_ = Role::Follower;
    int leaderId_ = 0;
    int commitIndex_ = 0;
    int lastApplied_ = 0;
    Time electionDeadline_ = 0;
    Time nextHeartbeat_ = 0;
    std::set<int> votes_;
    std::map<int, int> nextIndex_;
    std::map<int, int> matchIndex_;
    std::map<char, int> kv_;           // the state machine: a key-value map
    std::map<int, int> lastRequest_;   // client -> last request applied (duplicates)
};

// ---- shared rules -------------------------------------------------------------

inline void Node::persist()
{
    if (faults_.lazyPersist) {  // FAULT: the write is postponed (see tick)
        dirty_ = true;
        return;
    }
    disk_ = st_;
}

inline void Node::restart()
{
    st_ = disk_;  // only what reached the disk comes back
    refreshConfig();
    dirty_ = false;
    up_ = true;
    role_ = Role::Follower;
    leaderId_ = 0;
    commitIndex_ = 0;
    lastApplied_ = 0;
    votes_.clear();
    nextIndex_.clear();
    matchIndex_.clear();
    kv_.clear();
    lastRequest_.clear();
    resetElectionTimer();
    note("restarted from disk: term %d, votedFor %d, %d log entries",
         st_.currentTerm, st_.votedFor, lastIndex());
}

// "If RPC request or response contains term T > currentTerm:
//  set currentTerm = T, convert to follower" (all servers, Figure 2)
inline void Node::becomeFollower(int term)
{
    if (term > st_.currentTerm) {
        st_.currentTerm = term;
        st_.votedFor = 0;
        persist();
    }
    if (role_ != Role::Follower) {
        note("-> follower");
        resetElectionTimer();  // a fresh timer, not one left over from the old role
    }
    role_ = Role::Follower;
}

inline void Node::resetElectionTimer()
{
    const auto fixed = opt_.fixedTimeout.find(id_);
    const Time t = fixed != opt_.fixedTimeout.end()
                       ? fixed->second
                       : env_.rng().range(opt_.electionMin, opt_.electionMax);
    electionDeadline_ = env_.now() + t;
}

inline void Node::tick()
{
    if (!up_) {
        return;
    }
    const Time now = env_.now();
    if (dirty_ && now % 100 == 0) {  // the background writer of the lazyPersist fault
        disk_ = st_;
        dirty_ = false;
    }
    if (role_ == Role::Leader) {
        if (now >= nextHeartbeat_) {
            broadcastAppendEntries();
        }
    } else if (now >= electionDeadline_ && isMember(id_)) {
        startElection();
    }
}

inline void Node::receive(const Message& m)
{
    if (!up_) {
        return;
    }
    if (m.type == MsgType::ClientRequest) {
        handleClientRequest(m);
        return;
    }
    if (m.term > st_.currentTerm) {
        becomeFollower(m.term);
        leaderId_ = 0;
    }
    switch (m.type) {
    case MsgType::RequestVote: handleRequestVote(m); break;
    case MsgType::VoteReply: handleVoteReply(m); break;
    case MsgType::AppendEntries: handleAppendEntries(m); break;
    case MsgType::AppendReply: handleAppendReply(m); break;
    default: break;
    }
}

// Apply committed entries in log order to the key-value state machine.
inline void Node::applyCommitted()
{
    while (lastApplied_ < commitIndex_) {
        ++lastApplied_;
        const Entry& e = st_.log[lastApplied_ - 1];
        bool duplicate = false;
        if (e.client != 0) {
            duplicate = lastRequest_[e.client] >= e.request;
            lastRequest_[e.client] = std::max(lastRequest_[e.client], e.request);
        }
        if (e.type == OpType::Put && !duplicate) {
            kv_[e.key] = e.value;
        }
        env_.applied(id_, lastApplied_, e);
        // Only the leader that accepted the request (same term) answers the client.
        if (role_ == Role::Leader && e.term == st_.currentTerm && e.client != 0 && !duplicate) {
            Message r;
            r.type = MsgType::ClientReply;
            r.to = e.client;
            r.ok = true;
            r.op = e;
            r.result = kv_.count(e.key) ? kv_.at(e.key) : 0;
            send(r);
        }
    }
}

inline void Node::send(Message m)
{
    m.from = id_;
    if (m.type != MsgType::ClientReply) {
        m.term = st_.currentTerm;
    }
    env_.send(m);
}

inline void Node::note(const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    env_.trace(id_, st_.currentTerm, role_, buf);
}

} // namespace raft

#include "raft_election.h"
#include "raft_membership.h"
#include "raft_replication.h"
