// bcache.cpp - F3-31: a block cache with LRU replacement and write-back, tested against a
// reference model (curriculum B14, second acceptance test). Host C++; the kernel version has
// the same structure with a lock and a sleep queue instead of single-threaded calls.
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <list>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

constexpr std::size_t BLOCK = 512;

class RamDisk                            // the "device": counts every transfer
{
public:
    explicit RamDisk(std::size_t blocks) : data_(blocks * BLOCK, 0) {}
    void read(std::uint32_t b, std::uint8_t* out) { std::memcpy(out, &data_[b * BLOCK], BLOCK); ++reads; }
    void write(std::uint32_t b, const std::uint8_t* in) { std::memcpy(&data_[b * BLOCK], in, BLOCK); ++writes; }
    const std::vector<std::uint8_t>& bytes() const { return data_; }
    std::size_t reads = 0, writes = 0;
private:
    std::vector<std::uint8_t> data_;
};

class BlockCache
{
public:
    BlockCache(RamDisk& disk, std::size_t capacity, bool write_through)
        : disk_(disk), capacity_(capacity), write_through_(write_through) {}

    void read(std::uint32_t b, std::size_t off, std::size_t len, std::uint8_t* out)
    {
        Buf& buf = get(b);
        std::memcpy(out, buf.data.data() + off, len);
    }

    void write(std::uint32_t b, std::size_t off, std::size_t len, const std::uint8_t* in)
    {
        Buf& buf = get(b);
        std::memcpy(buf.data.data() + off, in, len);
        buf.dirty = true;                // the disk copy is now out of date
        trace(b, "write");
        if (write_through_) flush(buf);
    }

    void sync()                          // write every dirty buffer back, in block order
    {
        std::vector<Buf*> dirty;
        for (Buf& buf : lru_) if (buf.dirty) dirty.push_back(&buf);
        std::sort(dirty.begin(), dirty.end(), [](Buf* x, Buf* y) { return x->block < y->block; });
        for (Buf* buf : dirty) flush(*buf);
    }

    void show_trace(std::uint32_t b) const
    {
        auto it = history_.find(b);
        if (it == history_.end()) return;
        std::printf("history of block %u (oldest first):\n", b);
        for (const std::string& e : it->second) std::printf("  %s\n", e.c_str());
    }

    std::size_t hits = 0, misses = 0, writebacks = 0;

private:
    struct Buf
    {
        std::uint32_t block;
        bool dirty;
        std::array<std::uint8_t, BLOCK> data;
    };

    Buf& get(std::uint32_t b)
    {
        auto it = index_.find(b);
        if (it != index_.end()) {        // hit: move to the front (most recently used)
            ++hits;
            last_was_miss_ = false;
            lru_.splice(lru_.begin(), lru_, it->second);
            return lru_.front();
        }
        ++misses;
        last_was_miss_ = true;
        if (lru_.size() == capacity_) {  // full: evict the least recently used buffer
            Buf& victim = lru_.back();
            trace(victim.block, victim.dirty ? "evict (dirty: written back)" : "evict (clean: dropped)");
            if (victim.dirty) flush(victim);
            index_.erase(victim.block);
            lru_.pop_back();
        }
        lru_.push_front(Buf{b, false, {}});
        disk_.read(b, lru_.front().data.data());
        trace(b, "load from disk");
        index_[b] = lru_.begin();
        return lru_.front();
    }

    void flush(Buf& buf)
    {
        disk_.write(buf.block, buf.data.data());
        buf.dirty = false;
        ++writebacks;
    }

    void trace(std::uint32_t b, const char* what)
    {
        auto& h = history_[b];
        h.push_back(std::string(what) + " (op " + std::to_string(op) + ")");
        if (h.size() > 6) h.erase(h.begin());   // keep the last six events per block
    }

public:
    std::size_t op = 0;
private:
    RamDisk& disk_;
    std::size_t capacity_;
    bool write_through_;
    bool last_was_miss_ = false;         // whether the last get() had to load from disk
    std::list<Buf> lru_;
    std::unordered_map<std::uint32_t, std::list<Buf>::iterator> index_;
    std::unordered_map<std::uint32_t, std::vector<std::string>> history_;
};

// Random reads and writes over `blocks` blocks; every read is checked against the model.
// hot > 0 sends 80 % of the operations to the first `hot` blocks (a workload with locality).
static bool run(const char* label, std::size_t cache_blocks, std::size_t blocks, std::size_t ops,
                bool write_through, std::size_t hot)
{
    RamDisk disk(blocks);
    BlockCache cache(disk, cache_blocks, write_through);
    std::vector<std::uint8_t> model(blocks * BLOCK, 0);
    std::mt19937 rng(304);               // fixed seed: every run does the same operations
    std::uniform_int_distribution<std::uint32_t> any(0, static_cast<std::uint32_t>(blocks - 1));
    std::uniform_int_distribution<std::uint32_t> in_hot(0, static_cast<std::uint32_t>(hot ? hot - 1 : 0));
    std::uniform_int_distribution<int> pct(0, 99);
    std::vector<std::uint8_t> buf(BLOCK);
    for (std::size_t i = 0; i < ops; ++i) {
        cache.op = i;
        std::uint32_t b = (hot && pct(rng) < 80) ? in_hot(rng) : any(rng);
        std::size_t off = rng() % BLOCK;
        std::size_t len = 1 + rng() % (BLOCK - off);
        if (pct(rng) < 50) {
            cache.read(b, off, len, buf.data());
            if (std::memcmp(buf.data(), &model[b * BLOCK + off], len) != 0) {
                std::printf("%s: READ MISMATCH at op %zu, block %u, bytes %zu..%zu\n",
                            label, i, b, off, off + len - 1);
                cache.show_trace(b);
                return false;
            }
        } else {
            for (std::size_t k = 0; k < len; ++k) buf[k] = static_cast<std::uint8_t>(rng());
            cache.write(b, off, len, buf.data());
            std::memcpy(&model[b * BLOCK + off], buf.data(), len);
        }
    }
    std::size_t before_sync = disk.writes;
    cache.sync();
    bool same = disk.bytes() == model;
    std::printf("%-28s hits %6zu  misses %6zu  disk reads %6zu  disk writes %6zu (%zu by sync)  %s\n",
                label, cache.hits, cache.misses, disk.reads, disk.writes, disk.writes - before_sync,
                same ? "disk == model" : "DISK DIFFERS FROM MODEL");
    return same;
}

int main()
{
    const std::size_t cache_blocks = 64, blocks = 640, ops = 100000;   // blocks = 10 x cache size
    std::printf("cache %zu blocks of %zu bytes; disk %zu blocks (10x); %zu random operations\n",
                cache_blocks, BLOCK, blocks, ops);
    bool ok = run("write-back, uniform", cache_blocks, blocks, ops, false, 0);
    ok = run("write-through, uniform", cache_blocks, blocks, ops, true, 0) && ok;
    ok = run("write-back, 80% on 32 blocks", cache_blocks, blocks, ops, false, 32) && ok;
    ok = run("write-through, 80% on 32", cache_blocks, blocks, ops, true, 32) && ok;
    std::printf("B14 block cache test: %s\n", ok ? "PASS (contents match the model byte for byte)" : "FAIL");
    return ok ? 0 : 1;
}
