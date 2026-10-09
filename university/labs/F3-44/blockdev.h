// blockdev.h - F3-44: a disk image held in memory that records every block write and every
// cache flush, in order, to a "write stream" file. The crash tool replays a prefix of that
// stream to build the disk as it could look after a power cut.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace os401 {

inline std::uint16_t get16(const std::uint8_t* p) { return static_cast<std::uint16_t>(p[0] | p[1] << 8); }
inline std::uint32_t get32(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1]) << 8 |
           static_cast<std::uint32_t>(p[2]) << 16 | static_cast<std::uint32_t>(p[3]) << 24;
}
inline void put16(std::uint8_t* p, std::uint32_t v)
{
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
}
inline void put32(std::uint8_t* p, std::uint32_t v)
{
    put16(p, v);
    put16(p + 2, v >> 16);
}

using Bytes = std::vector<std::uint8_t>;

inline Bytes loadFile(const std::string& path)
{
    std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path.c_str(), "rb"), &std::fclose);
    if (!f) throw std::runtime_error("cannot open " + path);
    Bytes b;
    std::uint8_t buf[65536];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f.get())) > 0) b.insert(b.end(), buf, buf + n);
    return b;
}

inline void saveFile(const std::string& path, const Bytes& b)
{
    std::unique_ptr<FILE, int (*)(FILE*)> f(std::fopen(path.c_str(), "wb"), &std::fclose);
    if (!f || std::fwrite(b.data(), 1, b.size(), f.get()) != b.size())
        throw std::runtime_error("cannot write " + path);
}

// Write-stream records: 'W' u32 block, then blockSize bytes; 'F' (flush);
// 'P' u16 length, then text (a promise: "this fsync has returned").
class BlockDev
{
public:
    BlockDev(const std::string& image, std::uint32_t blockSize, const std::string& streamPath)
        : img_(loadFile(image)), bs_(blockSize), log_(nullptr, &std::fclose)
    {
        if (img_.size() % bs_ != 0) throw std::runtime_error("image size is not whole blocks");
        if (!streamPath.empty()) {
            log_.reset(std::fopen(streamPath.c_str(), "wb"));
            if (!log_) throw std::runtime_error("cannot create " + streamPath);
        }
    }
    std::uint32_t blockSize() const { return bs_; }
    std::uint32_t blockCount() const { return static_cast<std::uint32_t>(img_.size() / bs_); }
    void read(std::uint32_t blk, std::uint8_t* out) const
    {
        check(blk);
        std::memcpy(out, &img_[static_cast<std::size_t>(blk) * bs_], bs_);
    }
    void write(std::uint32_t blk, const std::uint8_t* in)
    {
        check(blk);
        std::memcpy(&img_[static_cast<std::size_t>(blk) * bs_], in, bs_);
        ++writes_;
        if (log_) {
            std::uint8_t h[5] = {'W'};
            put32(h + 1, blk);
            std::fwrite(h, 1, 5, log_.get());
            std::fwrite(in, 1, bs_, log_.get());
        }
    }
    void flush()
    {
        ++flushes_;
        if (log_) std::fputc('F', log_.get());
    }
    void promise(const std::string& text)
    {
        if (log_) {
            std::uint8_t h[3] = {'P'};
            put16(h + 1, static_cast<std::uint32_t>(text.size()));
            std::fwrite(h, 1, 3, log_.get());
            std::fwrite(text.data(), 1, text.size(), log_.get());
        }
    }
    void save(const std::string& path) const { saveFile(path, img_); }
    unsigned long writes() const { return writes_; }
    unsigned long flushes() const { return flushes_; }

private:
    void check(std::uint32_t blk) const
    {
        if (blk >= blockCount()) throw std::runtime_error("block " + std::to_string(blk) + " out of range");
    }
    Bytes img_;
    std::uint32_t bs_;
    std::unique_ptr<FILE, int (*)(FILE*)> log_;
    unsigned long writes_ = 0;
    unsigned long flushes_ = 0;
};

} // namespace os401
