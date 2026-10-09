// raft_membership.h - configurations, majorities and joint consensus
// (Raft paper, section 6). A server always uses the latest configuration in
// its log, committed or not.
#pragma once
#include "raft.h"

namespace raft {

// Called whenever the log changes (append, truncate, restart).
inline void Node::refreshConfig()
{
    for (int i = lastIndex(); i >= 1; --i) {
        if (st_.log[i - 1].type == OpType::Config) {
            cfg_ = st_.log[i - 1].config;
            return;
        }
    }
    cfg_ = boot_;
}

inline bool Node::isMember(int id) const
{
    const Config& c = config();
    const auto in = [id](const std::vector<int>& v) {
        return std::find(v.begin(), v.end(), id) != v.end();
    };
    return in(c.members) || in(c.joining);
}

// Everyone the leader talks to: the union of old and new configurations.
inline std::vector<int> Node::peers() const
{
    const Config& c = config();
    std::set<int> all(c.members.begin(), c.members.end());
    all.insert(c.joining.begin(), c.joining.end());
    all.erase(id_);
    return {all.begin(), all.end()};
}

// A majority of `members`; during joint consensus also a majority of `joining`.
inline bool Node::quorum(const std::set<int>& ids) const
{
    const auto majorityOf = [&ids](const std::vector<int>& group) {
        int yes = 0;
        for (int s : group) {
            yes += static_cast<int>(ids.count(s));
        }
        return 2 * yes > static_cast<int>(group.size());
    };
    const Config& c = config();
    return majorityOf(c.members) && (!c.joint() || majorityOf(c.joining));
}

// Leader only, after the commit index moved.
inline void Node::membershipStep()
{
    int cfgIndex = 0;
    for (int i = lastIndex(); i >= 1 && cfgIndex == 0; --i) {
        if (st_.log[i - 1].type == OpType::Config) {
            cfgIndex = i;
        }
    }
    if (cfgIndex == 0 || cfgIndex > commitIndex_) {
        return;  // no configuration entry, or the latest one is not committed yet
    }
    const Config& c = config();
    if (c.joint()) {  // C_old,new is committed: now propose C_new alone
        Entry e;
        e.type = OpType::Config;
        e.config.members = c.joining;
        note("joint configuration committed; appends C_new %s", listIds(c.joining).c_str());
        appendAsLeader(e);
    } else if (!isMember(id_)) {  // C_new committed and I am not in it
        note("C_new committed without me: steps down");
        role_ = Role::Follower;
        leaderId_ = 0;
    }
}

} // namespace raft
