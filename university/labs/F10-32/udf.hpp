// udf.hpp - F10-32 Listing 1: writer for the university's self-describing binary flight log
// ("UDF"). Every record starts with two marker bytes and a type byte. Type 128 is the
// format record (FMT): it declares, for one other type, its total record length, a 4-letter
// name, one format letter per field and the field labels. A reader needs no prior knowledge
// of the message set: it learns it from the FMT records at the start of the file. The idea is
// the one ArduPilot's DataFlash logs use; the marker bytes, letters and record layout below are
// OUR OWN choices (see the chapter's unverified box for the real ones).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace udf {

inline constexpr std::uint8_t kHead1 = 0xA5, kHead2 = 0x5A, kFmtType = 128;

// format letters -> size in bytes
inline int letterSize(char c)
{
    switch (c) {
    case 'B': case 'b': return 1;
    case 'H': case 'h': return 2;
    case 'I': case 'i': case 'f': return 4;
    case 'Q': return 8;
    case 'n': return 4;      // char[4]
    case 'N': return 16;     // char[16]
    case 'Z': return 64;     // char[64]
    default: throw std::runtime_error(std::string("unknown format letter ") + c);
    }
}

class Writer {
public:
    explicit Writer(const char* path) : f_(std::fopen(path, "wb"))
    {
        if (f_ == nullptr) {
            throw std::runtime_error("cannot open log file");
        }
        // the FMT record describes itself first
        define(kFmtType, "FMT", "BBnNZ", "Type,Length,Name,Format,Columns");
    }
    ~Writer() { std::fclose(f_); }
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    // declare a message type: writes its FMT record
    void define(std::uint8_t type, const char* name, const char* format, const char* labels)
    {
        int len = 3;
        for (const char* p = format; *p != '\0'; ++p) {
            len += letterSize(*p);
        }
        Record r(kFmtType);
        r.u8(type);
        r.u8(static_cast<std::uint8_t>(len));
        r.chars(name, 4);
        r.chars(format, 16);
        r.chars(labels, 64);
        write(r);
    }

    struct Record {
        explicit Record(std::uint8_t type) : bytes{kHead1, kHead2, type} {}
        void u8(std::uint8_t v) { bytes.push_back(v); }
        void i8(std::int8_t v) { bytes.push_back(static_cast<std::uint8_t>(v)); }
        template <class T>
        void raw(T v)
        {
            std::uint8_t b[sizeof(T)];
            std::memcpy(b, &v, sizeof(T));
            bytes.insert(bytes.end(), b, b + sizeof(T));
        }
        void chars(const char* s, std::size_t n)
        {
            for (std::size_t i = 0; i < n; ++i) {
                bytes.push_back(static_cast<std::uint8_t>(i < std::strlen(s) ? s[i] : 0));
            }
        }
        std::vector<std::uint8_t> bytes;
    };

    void write(const Record& r)
    {
        std::fwrite(r.bytes.data(), 1, r.bytes.size(), f_);
        bytesWritten += r.bytes.size();
    }
    std::size_t bytesWritten = 0;

private:
    std::FILE* f_;
};

}  // namespace udf
