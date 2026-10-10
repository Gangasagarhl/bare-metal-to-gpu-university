// cdr_bytes.cpp - turning a message into bytes, in the style of CDR (F9-40).
// The alignment rule used here (each primitive aligned to its own size, measured from the
// start of the payload; little-endian; a string as a 4-byte length that counts the final NUL,
// then the characters and the NUL) is the university's reading of CDR FROM MEMORY:
// see the unverified box in the chapter. What is real: the bytes this program prints.
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

class Writer {
public:
    template <typename T> void put(T value)
    {
        static_assert(std::endian::native == std::endian::little, "this sketch assumes a little-endian host");
        align(sizeof(T));
        unsigned char b[sizeof(T)];
        std::memcpy(b, &value, sizeof(T));
        buf_.insert(buf_.end(), b, b + sizeof(T));
    }
    void putString(const std::string& s)
    {
        put<std::uint32_t>(static_cast<std::uint32_t>(s.size() + 1));
        buf_.insert(buf_.end(), s.begin(), s.end());
        buf_.push_back(0);
    }
    const std::vector<unsigned char>& bytes() const { return buf_; }
    int padding() const { return padding_; }
private:
    void align(std::size_t n)
    {
        while (buf_.size() % n != 0) { buf_.push_back(0); ++padding_; }
    }
    std::vector<unsigned char> buf_;
    int padding_ = 0;
};

int main()
{
    // A tiny "range reading" message: frame name, sequence number, one range in metres.
    Writer w;
    w.putString("laser");                 // 4 + 6 bytes
    w.put<std::uint16_t>(7);              // seq: aligned to 2
    w.put<double>(1.25);                  // range: aligned to 8
    const auto& b = w.bytes();
    std::printf("payload %zu bytes (%d padding)\n", b.size(), w.padding());
    for (std::size_t i = 0; i < b.size(); ++i)
        std::printf("%02x%s", b[i], (i % 8 == 7 || i + 1 == b.size()) ? "\n" : " ");
    return 0;
}
