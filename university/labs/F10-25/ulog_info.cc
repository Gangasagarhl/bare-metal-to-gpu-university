// ulog_info.cc - prints what is inside a ULog-style file: header, message formats,
// information, parameters, how many records each topic has, and the logged strings.
//   ulog_info <file.ulg>            one log
//   ulog_info <a.ulg> <b.ulg>       the parameters that differ between two logs
#include <cstdio>
#include <exception>
#include <string>

#include "ulog_lite.h"

namespace {

void describe(const ulog_lite::Log& log, const char* path)
{
    std::printf("file %s: version %u, start %llu us\n", path, log.version,
                static_cast<unsigned long long>(log.startUs));
    std::printf("messages by type:");
    for (const auto& [type, count] : log.messageCounts) {
        std::printf(" '%c' %d", type, count);
    }
    std::printf("\n\nformats and record counts:\n");
    for (const auto& [topic, fields] : log.formats) {
        std::printf("  %-20s %5zu records:", topic.c_str(), log.rows(topic));
        for (const auto& f : fields) {
            std::printf(" %s %s;", f.type.c_str(), f.name.c_str());
        }
        std::printf("\n");
    }
    std::printf("\ninformation:\n");
    for (const auto& [k, v] : log.infos) {
        std::printf("  %s = %s\n", k.c_str(), v.c_str());
    }
    std::printf("\nparameters:\n");
    for (const auto& [k, v] : log.params) {
        std::printf("  %-16s %8.3f\n", k.c_str(), static_cast<double>(v));
    }
    std::printf("\nlogged messages:\n");
    for (const auto& [t, s] : log.texts) {
        std::printf("  %8.2f s  %s\n", static_cast<double>(t) * 1e-6, s.c_str());
    }
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (argc == 2) {
            describe(ulog_lite::read(argv[1]), argv[1]);
            return 0;
        }
        if (argc == 3) {
            const auto a = ulog_lite::read(argv[1]);
            const auto b = ulog_lite::read(argv[2]);
            std::printf("%-16s %10s %10s\n", "parameter", argv[1], argv[2]);
            int differ = 0;
            for (const auto& [k, va] : a.params) {
                float vb = 0.0f;
                if (!b.param(k, vb) || vb != va) {
                    std::printf("%-16s %10.3f %10.3f\n", k.c_str(), static_cast<double>(va),
                                static_cast<double>(vb));
                    ++differ;
                }
            }
            std::printf("%d parameter(s) differ\n", differ);
            return 0;
        }
        std::fprintf(stderr, "usage: ulog_info <file.ulg> [<other.ulg>]\n");
        return 2;
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
        return 1;
    }
}
