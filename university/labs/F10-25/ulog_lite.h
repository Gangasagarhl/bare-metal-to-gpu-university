// ulog_lite.h - a writer and a reader for this course's "ULog-style" binary log.
// The layout is modelled on the structure that the PX4 documentation gives for ULog
// (a file header, then a definitions section with message formats, information and
// parameters, then a data section of subscriptions, data and logged strings). It was
// written from memory in a build without internet: byte-level compatibility with
// PX4's tools is NOT verified (see the chapter's unverified box).
//
//   file header : "ULog" 0x01 0x12 0x35, version byte, uint64 start time (us)  = 16 bytes
//   each message: uint16 payload size, uint8 type, payload
//   'F' format  : "name:type field;type field;..."
//   'I' info    : uint8 key length, key ("type name"), value bytes
//   'P' param   : same encoding as 'I', type float here
//   'A' add     : uint8 multi id, uint16 msg id, topic name
//   'D' data    : uint16 msg id, the fields packed in format order (little-endian)
//   'L' string  : uint8 level, uint64 timestamp, text
#pragma once

#include <cstdint>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ulog_lite {

inline const std::uint8_t kMagic[7] = {'U', 'L', 'o', 'g', 0x01, 0x12, 0x35};

inline std::size_t typeSize(const std::string& t)
{
    if (t == "uint64_t") {
        return 8;
    }
    if (t == "float" || t == "int32_t") {
        return 4;
    }
    if (t == "uint8_t") {
        return 1;
    }
    throw std::runtime_error("unsupported type " + t);
}

class Writer
{
public:
    Writer(const std::string& path, std::uint64_t startUs) : out_(path, std::ios::binary)
    {
        out_.write(reinterpret_cast<const char*>(kMagic), sizeof kMagic);
        out_.put(1);   // version
        put(startUs);
    }

    void format(const std::string& def) { message('F', def); }

    void info(const std::string& key, const std::string& value)
    {
        message('I', keyed("char[" + std::to_string(value.size()) + "] " + key, value));
    }

    void param(const std::string& name, float v)
    {
        std::string bytes(sizeof v, '\0');
        std::memcpy(bytes.data(), &v, sizeof v);
        message('P', keyed("float " + name, bytes));
    }

    std::uint16_t addLogged(const std::string& topic)
    {
        const auto id = next_++;
        std::string p(1, '\0');
        p.append(reinterpret_cast<const char*>(&id), 2);
        p += topic;
        message('A', p);
        return id;
    }

    void data(std::uint16_t id, const std::string& packedFields)
    {
        std::string p(reinterpret_cast<const char*>(&id), 2);
        message('D', p + packedFields);
    }

    void text(std::uint8_t level, std::uint64_t tUs, const std::string& s)
    {
        std::string p(1, static_cast<char>(level));
        p.append(reinterpret_cast<const char*>(&tUs), 8);
        message('L', p + s);
    }

private:
    template <typename T>
    void put(T v)
    {
        out_.write(reinterpret_cast<const char*>(&v), sizeof v);
    }

    static std::string keyed(const std::string& key, const std::string& value)
    {
        return std::string(1, static_cast<char>(key.size())) + key + value;
    }

    void message(char type, const std::string& payload)
    {
        put(static_cast<std::uint16_t>(payload.size()));
        out_.put(type);
        out_.write(payload.data(), static_cast<std::streamsize>(payload.size()));
    }

    std::ofstream out_;
    std::uint16_t next_ = 0;
};

// Packs fields for a 'D' message, in the order of the format.
class Pack
{
public:
    template <typename T>
    Pack& operator<<(T v)
    {
        s_.append(reinterpret_cast<const char*>(&v), sizeof v);
        return *this;
    }
    const std::string& str() const { return s_; }

private:
    std::string s_;
};

struct Field
{
    std::string type;
    std::string name;
    std::size_t offset;
};

struct Log
{
    std::uint8_t version = 0;
    std::uint64_t startUs = 0;
    std::map<std::string, std::vector<Field>> formats;            // topic -> fields
    std::vector<std::pair<std::string, std::string>> infos;       // key -> value
    std::vector<std::pair<std::string, float>> params;
    std::map<std::uint16_t, std::string> subscriptions;           // msg id -> topic
    std::map<std::string, std::vector<std::string>> data;         // topic -> payloads
    std::vector<std::pair<std::uint64_t, std::string>> texts;
    std::map<char, int> messageCounts;

    double get(const std::string& topic, std::size_t row, const std::string& field) const
    {
        const std::string& p = data.at(topic).at(row);
        for (const Field& f : formats.at(topic)) {
            if (f.name != field) {
                continue;
            }
            const char* b = p.data() + f.offset;
            if (f.type == "uint64_t") {
                std::uint64_t v;
                std::memcpy(&v, b, 8);
                return static_cast<double>(v);
            }
            if (f.type == "float") {
                float v;
                std::memcpy(&v, b, 4);
                return v;
            }
            if (f.type == "int32_t") {
                std::int32_t v;
                std::memcpy(&v, b, 4);
                return v;
            }
            return static_cast<std::uint8_t>(*b);
        }
        throw std::runtime_error("no field " + field + " in " + topic);
    }

    std::size_t rows(const std::string& topic) const
    {
        auto it = data.find(topic);
        return it == data.end() ? 0 : it->second.size();
    }

    bool param(const std::string& name, float& v) const
    {
        for (const auto& p : params) {
            if (p.first == name) {
                v = p.second;
                return true;
            }
        }
        return false;
    }
};

inline Log read(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::string all((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (all.size() < 16 || std::memcmp(all.data(), kMagic, sizeof kMagic) != 0) {
        throw std::runtime_error(path + ": not a ULog-style file (magic bytes differ)");
    }
    Log log;
    log.version = static_cast<std::uint8_t>(all[7]);
    std::memcpy(&log.startUs, all.data() + 8, 8);
    std::size_t pos = 16;
    while (pos + 3 <= all.size()) {
        std::uint16_t size;
        std::memcpy(&size, all.data() + pos, 2);
        const char type = all[pos + 2];
        if (pos + 3 + size > all.size()) {
            throw std::runtime_error("truncated message at offset " + std::to_string(pos));
        }
        const std::string p = all.substr(pos + 3, size);
        pos += 3 + size;
        ++log.messageCounts[type];
        if (type == 'F') {
            const auto colon = p.find(':');
            std::vector<Field> fields;
            std::istringstream fs(p.substr(colon + 1));
            std::string item;
            std::size_t off = 0;
            while (std::getline(fs, item, ';')) {
                const auto sp = item.find(' ');
                Field f{item.substr(0, sp), item.substr(sp + 1), off};
                off += typeSize(f.type);
                fields.push_back(f);
            }
            log.formats[p.substr(0, colon)] = fields;
        } else if (type == 'I' || type == 'P') {
            const auto klen = static_cast<std::uint8_t>(p[0]);
            const std::string key = p.substr(1, klen);
            const std::string value = p.substr(1 + klen);
            const std::string name = key.substr(key.find(' ') + 1);
            if (type == 'I') {
                log.infos.emplace_back(name, value);
            } else {
                float v;
                std::memcpy(&v, value.data(), 4);
                log.params.emplace_back(name, v);
            }
        } else if (type == 'A') {
            std::uint16_t id;
            std::memcpy(&id, p.data() + 1, 2);
            log.subscriptions[id] = p.substr(3);
        } else if (type == 'D') {
            std::uint16_t id;
            std::memcpy(&id, p.data(), 2);
            log.data[log.subscriptions.at(id)].push_back(p.substr(2));
        } else if (type == 'L') {
            std::uint64_t t;
            std::memcpy(&t, p.data() + 1, 8);
            log.texts.emplace_back(t, p.substr(9));
        }
    }
    return log;
}

} // namespace ulog_lite
