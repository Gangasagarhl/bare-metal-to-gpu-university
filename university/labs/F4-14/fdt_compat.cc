// fdt_compat.cc - stage (a) for a board: list every devicetree node that has a "compatible"
// property, with its strings (most specific first), from a flattened devicetree blob (DTB).
// Format as assumed here (check against the Devicetree Specification, "Flattened Devicetree
// (DTB) Format", title only in this build): a big-endian header (magic 0xd00dfeed, total size,
// offset of the structure block, offset of the strings block, ...), then 32-bit tokens:
// BEGIN_NODE (1) + name, END_NODE (2), PROP (3) + length + name offset + value, NOP (4), END (9).
// Self-check: the walk must end on END with every node closed.
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static uint32_t be32(const std::vector<unsigned char>& b, std::size_t o)
{
    return uint32_t{b[o]} << 24 | uint32_t{b[o + 1]} << 16 | uint32_t{b[o + 2]} << 8 | b[o + 3];
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "usage: fdt_compat <file.dtb>\n";
        return 2;
    }
    std::ifstream f(argv[1], std::ios::binary);
    const std::vector<unsigned char> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (b.size() < 40 || be32(b, 0) != 0xd00dfeedu) {
        std::cerr << "not a DTB\n";
        return 1;
    }
    const uint32_t off_struct = be32(b, 8), off_strings = be32(b, 12);
    std::cout << "DTB: total size " << be32(b, 4) << " (file " << b.size() << " bytes), version " << be32(b, 20)
              << '\n';
    std::vector<std::string> path;
    std::size_t p = off_struct;
    int nodes = 0, with_compat = 0;
    bool ended = false;
    struct Entry { std::string first, stem, strings; int count; };
    std::vector<Entry> found;
    while (p + 4 <= b.size() && !ended) {
        const uint32_t tok = be32(b, p);
        p += 4;
        if (tok == 1) {                                     // BEGIN_NODE
            std::string name;
            while (b.at(p)) name += static_cast<char>(b[p++]);
            p = (p + 1 + 3) & ~std::size_t{3};
            path.push_back(name);
            ++nodes;
        } else if (tok == 2) {                              // END_NODE
            path.pop_back();
        } else if (tok == 3) {                              // PROP
            const uint32_t len = be32(b, p), nameoff = be32(b, p + 4);
            p += 8;
            std::string pname;
            for (std::size_t s = off_strings + nameoff; b.at(s); ++s) pname += static_cast<char>(b[s]);
            if (pname == "compatible") {
                std::string where, strings, cur;
                for (std::size_t i = 1; i < path.size(); ++i) where += "/" + path[i];
                for (std::size_t i = 0; i < len; ++i) {
                    if (b[p + i] == 0) { strings += " \"" + cur + "\""; cur.clear(); }
                    else cur += static_cast<char>(b[p + i]);
                }
                // Consecutive nodes with the same name stem and the same strings print as one line.
                const std::string stem = where.substr(0, where.find('@'));
                if (!found.empty() && found.back().stem == stem && found.back().strings == strings) {
                    ++found.back().count;
                } else {
                    found.push_back({where.empty() ? "/" : where, stem, strings, 1});
                }
                ++with_compat;
            }
            p = (p + len + 3) & ~std::size_t{3};
        } else if (tok == 4) {                              // NOP
        } else if (tok == 9) {                              // END
            ended = true;
        } else {
            std::cerr << "unknown token " << tok << " at offset " << p - 4 << '\n';
            return 1;
        }
    }
    for (const auto& e : found) {
        std::cout << e.first << ":" << e.strings;
        if (e.count > 1) std::cout << "   (and " << e.count - 1 << " more nodes " << e.stem << "@... like it)";
        std::cout << '\n';
    }
    std::cout << nodes << " nodes, " << with_compat << " with a compatible property; walk "
              << (ended && path.empty() ? "ended cleanly on END" : "DID NOT END CLEANLY") << '\n';
    return ended && path.empty() ? 0 : 1;
}
