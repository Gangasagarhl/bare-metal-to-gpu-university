// journal.h - F3-45: a JBD2-compatible metadata journal under the ext2 writer of F3-44
// (milestone FS2, "ordered" mode), and journal recovery (replay at mount).
// On-disk journal structures: Linux kernel documentation "ext4 Data Structures and
// Algorithms", journal (jbd2) section (chapter D1); fields are big-endian.
#pragma once
#include "../F3-44/ext2w.h"
#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace os401 {

constexpr std::uint32_t kJMagic = 0xC03B3998;
constexpr std::uint32_t kDescriptor = 1, kCommit = 2, kSuperV1 = 3, kSuperV2 = 4, kRevoke = 5;  // block types
constexpr std::uint32_t kEscape = 1, kSameUuid = 2, kLastTag = 8;                           // tag flags
constexpr std::uint32_t kRecoverFlag = 0x4;          // ext3/4 incompat "needs_recovery"

inline std::uint32_t be32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) << 24 | static_cast<std::uint32_t>(p[1]) << 16 |
           static_cast<std::uint32_t>(p[2]) << 8 | p[3];
}
inline void putBe32(std::uint8_t* p, std::uint32_t v)
{
    p[0] = static_cast<std::uint8_t>(v >> 24);
    p[1] = static_cast<std::uint8_t>(v >> 16);
    p[2] = static_cast<std::uint8_t>(v >> 8);
    p[3] = static_cast<std::uint8_t>(v);
}
inline void putBe16(std::uint8_t* p, std::uint32_t v)
{
    p[0] = static_cast<std::uint8_t>(v >> 8);
    p[1] = static_cast<std::uint8_t>(v);
}
inline void header(std::uint8_t* b, std::uint32_t type, std::uint32_t seq)
{
    putBe32(b, kJMagic);
    putBe32(b + 4, type);
    putBe32(b + 8, seq);
}

// Sets or clears needs_recovery in the primary superblock (1 KiB blocks: device block 1).
inline void setRecoverFlag(BlockDev& dev, bool on)
{
    Bytes b(dev.blockSize());
    dev.read(1, b.data());
    const std::uint32_t incompat = get32(&b[96]);
    put32(&b[96], on ? (incompat | kRecoverFlag) : (incompat & ~kRecoverFlag));
    dev.write(1, b.data());
}

struct ReplayReport
{
    int transactions = 0;
    int blocks = 0;
    int revoked = 0;
    std::uint32_t nextSequence = 0;
};

// Recovery: scan from s_start for transactions with consecutive sequence numbers; a
// transaction counts only if its commit block is there. Revoked blocks are skipped.
inline ReplayReport recover(BlockDev& dev, const std::vector<std::uint32_t>& jmap)
{
    const std::uint32_t bs = dev.blockSize();
    Bytes jsb(bs), b(bs);
    dev.read(jmap[0], jsb.data());
    if (be32(&jsb[0]) != kJMagic || (be32(&jsb[4]) != kSuperV1 && be32(&jsb[4]) != kSuperV2))
        throw std::runtime_error("no journal superblock");
    if (be32(&jsb[36]) || (be32(&jsb[40]) & ~0x1u) || be32(&jsb[44]))     // only incompat "revoke"
        throw std::runtime_error("journal features this listing does not know");
    const std::uint32_t first = be32(&jsb[20]), maxlen = be32(&jsb[16]);
    ReplayReport r;
    r.nextSequence = be32(&jsb[24]);
    if (be32(&jsb[28]) == 0) {                              // s_start == 0: the log is empty
        setRecoverFlag(dev, false);
        dev.flush();
        return r;
    }
    struct Logged { std::uint32_t target, logPos, seq; bool escaped; };
    std::vector<Logged> logged, pending;
    std::map<std::uint32_t, std::uint32_t> revokedIn;       // block -> latest revoking sequence
    std::map<std::uint32_t, std::uint32_t> pendingRevokes;
    std::uint32_t pos = be32(&jsb[28]), seq = r.nextSequence;
    auto next = [&](std::uint32_t p) { return p + 1 < maxlen ? p + 1 : first; };
    for (std::uint32_t steps = 0; steps < maxlen; ++steps) {
        dev.read(jmap[pos], b.data());
        if (be32(&b[0]) != kJMagic || be32(&b[8]) != seq) break;      // end of the log
        const std::uint32_t type = be32(&b[4]);
        if (type == kDescriptor) {
            std::uint32_t off = 12;
            for (;;) {
                const std::uint32_t target = be32(&b[off]), flags = static_cast<std::uint32_t>(b[off + 6] << 8 | b[off + 7]);
                pos = next(pos);
                pending.push_back({target, pos, seq, (flags & kEscape) != 0});
                off += 8 + ((flags & kSameUuid) ? 0 : 16);
                if ((flags & kLastTag) || off + 8 > bs) break;
            }
            pos = next(pos);
        } else if (type == kRevoke) {
            const std::uint32_t used = be32(&b[12]);
            for (std::uint32_t off = 16; off + 4 <= used; off += 4) pendingRevokes[be32(&b[off])] = seq;
            pos = next(pos);
        } else if (type == kCommit) {                                  // the transaction is complete
            logged.insert(logged.end(), pending.begin(), pending.end());
            for (auto [blk, s] : pendingRevokes) revokedIn[blk] = std::max(revokedIn[blk], s);
            pending.clear();
            pendingRevokes.clear();
            ++r.transactions;
            ++seq;
            pos = next(pos);
        } else {
            break;
        }
    }
    for (const Logged& l : logged) {                                   // replay, oldest first
        auto rv = revokedIn.find(l.target);
        if (rv != revokedIn.end() && rv->second >= l.seq) {
            ++r.revoked;
            continue;
        }
        dev.read(jmap[l.logPos], b.data());
        if (l.escaped) putBe32(&b[0], kJMagic);                         // undo the escaping
        dev.write(l.target, b.data());
        ++r.blocks;
    }
    dev.flush();
    r.nextSequence = seq;
    putBe32(&jsb[24], seq);                                             // s_sequence
    putBe32(&jsb[28], 0);                                               // s_start: log empty
    dev.write(jmap[0], jsb.data());
    setRecoverFlag(dev, false);
    dev.flush();
    return r;
}

class JournalStore : public Store
{
public:
    JournalStore(BlockDev& dev, std::vector<std::uint32_t> jmap, bool flushBeforeCommit)
        : dev_(dev), jmap_(std::move(jmap)), bs_(dev.blockSize()), flushBeforeCommit_(flushBeforeCommit)
    {
        jsb_.resize(bs_);
        dev_.read(jmap_[0], jsb_.data());
        if (be32(&jsb_[0]) != kJMagic || be32(&jsb_[4]) != kSuperV2 || be32(&jsb_[12]) != bs_)
            throw std::runtime_error("expected a version 2 journal superblock with the file system's block size");
        if (be32(&jsb_[36]) || be32(&jsb_[40]) || be32(&jsb_[44]))
            throw std::runtime_error("journal features this listing does not write");
    }
    // Mount: replay what a crash left behind, then mark the file system "needs recovery".
    ReplayReport mount()
    {
        ReplayReport r = recover(dev_, jmap_);
        dev_.read(jmap_[0], jsb_.data());
        seq_ = be32(&jsb_[24]);
        setRecoverFlag(dev_, true);
        dev_.flush();
        return r;
    }
    void unmount()
    {
        commit();
        setRecoverFlag(dev_, false);
        dev_.flush();
    }

    void read(std::uint32_t b, std::uint8_t* out) override
    {
        auto it = tx_.find(b);
        if (it != tx_.end()) std::copy(it->second.begin(), it->second.end(), out);
        else dev_.read(b, out);
    }
    void meta(std::uint32_t b, const std::uint8_t* in) override
    {
        if (!tx_.count(b)) order_.push_back(b);
        tx_[b].assign(in, in + bs_);                        // kept in memory until commit
    }
    void data(std::uint32_t b, const std::uint8_t* in) override { dev_.write(b, in); }   // in place
    void order() override {}                                // the transaction is atomic as a whole
    void sync() override { commit(); }

    void commit()
    {
        if (tx_.empty()) return;
        const std::uint32_t first = be32(&jsb_[20]), maxlen = be32(&jsb_[16]);
        putBe32(&jsb_[24], seq_);
        putBe32(&jsb_[28], first);                          // s_start: the log begins here
        dev_.write(jmap_[0], jsb_.data());
        std::uint32_t pos = first;
        std::size_t i = 0;
        while (i < order_.size()) {                         // descriptor + its logged blocks
            Bytes desc(bs_, 0);
            header(desc.data(), kDescriptor, seq_);
            std::uint32_t off = 12, lastTag = 0;
            std::vector<Bytes> copies;
            for (bool firstTag = true; i < order_.size() && off + 8 + (firstTag ? 16 : 0) <= bs_; ++i) {
                Bytes copy = tx_[order_[i]];
                std::uint32_t flags = firstTag ? 0 : kSameUuid;
                if (be32(&copy[0]) == kJMagic) {
                    putBe32(&copy[0], 0);                   // escape a block that looks like ours
                    flags |= kEscape;
                }
                putBe32(&desc[off], order_[i]);                 // t_blocknr
                putBe16(&desc[off + 6], flags);                 // t_flags (t_checksum stays 0)
                lastTag = off;
                if (firstTag) std::copy(&jsb_[48], &jsb_[64], &desc[off + 8]);   // journal UUID
                off += 8 + (firstTag ? 16 : 0);
                copies.push_back(std::move(copy));
                firstTag = false;
            }
            desc[lastTag + 7] = static_cast<std::uint8_t>(desc[lastTag + 7] | kLastTag);
            if (pos + 1 + copies.size() + 1 > maxlen) throw std::runtime_error("transaction larger than the journal");
            dev_.write(jmap_[pos++], desc.data());
            for (const Bytes& c : copies) dev_.write(jmap_[pos++], c.data());
        }
        if (flushBeforeCommit_) dev_.flush();               // data and log durable before ...
        Bytes cb(bs_, 0);
        header(cb.data(), kCommit, seq_);
        putBe32(&cb[52], kFixedTime);                       // h_commit_sec (64-bit, low half)
        dev_.write(jmap_[pos], cb.data());                  // ... the commit block
        dev_.flush();
        for (std::uint32_t b : order_) dev_.write(b, tx_[b].data());      // checkpoint in place
        dev_.flush();
        putBe32(&jsb_[24], ++seq_);
        putBe32(&jsb_[28], 0);                              // log empty again
        dev_.write(jmap_[0], jsb_.data());
        dev_.flush();                                       // before the log area is reused
        ++commits_;
        logged_ += order_.size();
        tx_.clear();
        order_.clear();
    }
    unsigned long commits() const { return commits_; }
    unsigned long logged() const { return logged_; }

private:
    BlockDev& dev_;
    std::vector<std::uint32_t> jmap_;
    std::uint32_t bs_;
    bool flushBeforeCommit_;
    Bytes jsb_;
    std::uint32_t seq_ = 0;
    std::map<std::uint32_t, Bytes> tx_;
    std::vector<std::uint32_t> order_;
    unsigned long commits_ = 0, logged_ = 0;
};

// Journal block i lives in device block jmap[i]; the journal is inode s_journal_inum.
inline std::vector<std::uint32_t> journalMap(Ext2& fs, std::uint32_t bs)
{
    const std::uint32_t jino = fs.superField32(224);        // s_journal_inum
    if (!jino) throw std::runtime_error("no journal inode: not an ext3 image");
    std::vector<std::uint32_t> m;
    for (std::uint32_t i = 0; i < fs.sizeOf(jino) / bs; ++i) m.push_back(fs.blockOf(jino, i));
    return m;
}

} // namespace os401
