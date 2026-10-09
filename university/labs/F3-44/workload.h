// workload.h - F3-44: the random workload of the FS1 harness, shared with F3-45. It keeps a
// model of what every file must contain, and records a "promise" in the write stream each
// time an fsync of a file that will never change again has returned.
#pragma once
#include "ext2w.h"
#include <cstdio>
#include <functional>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace os401 {

struct Model
{
    std::map<std::string, int> names;        // path of a regular file -> content id
    std::map<int, Bytes> contents;           // content id -> expected bytes
    std::set<std::string> promised;          // fsynced files: never touched again
    std::vector<std::string> dirs{""};       // "" is the root directory
    int next = 0;
};

inline Bytes randomBytes(std::mt19937& rng, std::size_t n)
{
    Bytes b(n);
    for (auto& x : b) x = static_cast<std::uint8_t>(rng() >> 24);
    return b;
}

inline std::size_t randomSize(std::mt19937& rng)
{
    const unsigned r = rng() % 100;
    if (r < 70) return rng() % 8192;
    if (r < 95) return 8192 + rng() % 32768;
    return 270000 + rng() % 50000;           // beyond 268 KiB: needs the double indirect block
}

template <class C>
auto pick(std::mt19937& rng, const C& c)
{
    auto it = c.begin();
    std::advance(it, rng() % c.size());
    return *it;
}

// Runs `ops` random operations. afterOp() is called after each one (F3-45 commits there).
inline int runWorkload(Ext2& fs, BlockDev& dev, unsigned seed, int ops, bool promising,
                       const std::string& expect, const std::string& label,
                       const std::function<void()>& afterOp)
{
    std::mt19937 rng(seed);
    Model m;
    std::map<std::string, int> done;
    auto newPath = [&](char kind) {
        return pick(rng, m.dirs) + "/" + kind + std::to_string(m.next++);
    };
    auto scratch = [&]() {
        std::vector<std::string> s;
        for (const auto& [p, id] : m.names) {
            if (!m.promised.count(p)) s.push_back(p);
        }
        return s;
    };
    for (int i = 0; i < ops; ++i) {
        unsigned r = rng() % 100;
        const auto sc = scratch();
        if (fs.freeBlocks() < 1500 || fs.freeInodes() < 20) r = sc.empty() ? 200 : 60;   // make room: unlink
        if (r == 200) break;
        if ((r >= 30 && r < 94) && sc.empty()) r = 15;
        if (r < 15 || r >= 94) {                                   // a file that gets fsynced
            const std::string p = r < 15 ? newPath('p') : newPath('f');
            const int id = m.next++;
            fs.create(p);
            Bytes d = randomBytes(rng, randomSize(rng));
            std::uint32_t off = 0;
            if (rng() % 5 == 0) off = 1024 * (1 + rng() % 40);    // leave a hole: a sparse file
            fs.write(p, off, d);
            Bytes model(off, 0);
            model.insert(model.end(), d.begin(), d.end());
            m.names[p] = id;
            m.contents[id] = model;
            fs.fsync();
            if (r < 15 && promising) {
                m.promised.insert(p);
                dev.promise(std::to_string(id) + "\t" + p);       // "fsync has returned"
            }
            ++done[r < 15 && promising ? "create+write+fsync (promised)" : "create+write+fsync"];
        } else if (r < 30) {                                       // scratch file
            const std::string p = newPath('s');
            const int id = m.next++;
            fs.create(p);
            Bytes d = randomBytes(rng, randomSize(rng));
            fs.write(p, 0, d);
            m.names[p] = id;
            m.contents[id] = d;
            ++done["create+write"];
        } else if (r < 45) {                                       // append or overwrite
            const std::string p = pick(rng, sc);
            Bytes& c = m.contents[m.names[p]];
            const std::uint32_t off = static_cast<std::uint32_t>(rng() % (c.size() + 4096));
            Bytes d = randomBytes(rng, 1 + rng() % 20000);
            if (off + d.size() > 330000) {
                ++done["skipped (file would exceed 330000 bytes)"];
                continue;
            }
            fs.write(p, off, d);
            if (c.size() < off + d.size()) c.resize(off + d.size(), 0);
            std::copy(d.begin(), d.end(), c.begin() + off);
            ++done["write"];
        } else if (r < 55) {                                       // truncate
            const std::string p = pick(rng, sc);
            Bytes& c = m.contents[m.names[p]];
            const std::uint32_t size = c.empty() ? 0 : static_cast<std::uint32_t>(rng() % c.size());
            fs.truncate(p, size);
            c.resize(std::min<std::size_t>(c.size(), size));
            ++done["truncate"];
        } else if (r < 70) {                                       // unlink
            const std::string p = pick(rng, sc);
            fs.unlink(p);
            m.names.erase(p);
            ++done["unlink"];
        } else if (r < 80) {                                       // rename
            const std::string p = pick(rng, sc), q = newPath('r');
            fs.rename(p, q);
            m.names[q] = m.names[p];
            m.names.erase(p);
            ++done["rename"];
        } else if (r < 86) {                                       // hard link
            const std::string p = pick(rng, sc), q = newPath('h');
            fs.link(p, q);
            m.names[q] = m.names[p];
            ++done["link"];
        } else if (r < 90) {                                       // symbolic link
            fs.symlink(pick(rng, sc), newPath('l'));
            ++done["symlink"];
        } else {                                                   // mkdir
            if (m.dirs.size() >= 30) {
                ++done["skipped (30 directories reached)"];
                continue;
            }
            const std::string p = newPath('d');
            fs.mkdir(p);
            m.dirs.push_back(p);
            ++done["mkdir"];
        }
        afterOp();
    }
    fs.fsync();                                                    // clean "unmount"
    std::FILE* fin = std::fopen((expect + "/final.txt").c_str(), "w");
    std::FILE* fpr = std::fopen((expect + "/promised.txt").c_str(), "w");
    if (!fin || !fpr) throw std::runtime_error("cannot write to " + expect);
    for (const auto& [p, id] : m.names) {
        const std::string f = expect + "/c" + std::to_string(id);
        saveFile(f, m.contents[id]);
        std::fprintf(fin, "%s\t%s\n", p.c_str(), f.c_str());
        if (m.promised.count(p)) std::fprintf(fpr, "%d\t%s\t%s\n", id, p.c_str(), f.c_str());
    }
    std::fclose(fin);
    std::fclose(fpr);
    std::printf("workload seed %u, %s:\n", seed, label.c_str());
    for (const auto& [what, n] : done) std::printf("  %5d  %s\n", n, what.c_str());
    std::printf("  block writes %lu, flushes %lu; files %zu (%zu promised), directories %zu\n",
                dev.writes(), dev.flushes(), m.names.size(), m.promised.size(), m.dirs.size());
    return 0;
}

} // namespace os401
