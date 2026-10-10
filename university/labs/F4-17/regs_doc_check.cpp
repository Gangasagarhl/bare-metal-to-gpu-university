// regs_doc_check.cpp - checks two promises of the design note mechanically:
//  (1) every aspect row of curriculum section 5.4 is present and filled in;
//  (2) every register the driver source touches (rd/wr on BAR0, pci4 accessors on PCI
//      configuration registers) is listed in the note's register table with a source.
//   regs_doc_check                      design_note.txt and edu4.cc in the current folder
//   regs_doc_check <note> <driver.cc>   any other pair (the forensic lab uses this)
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>

static std::string slurp(const char* path)
{
    std::ifstream f(path);
    std::stringstream s;
    s << f.rdbuf();
    return s.str();
}

int main(int argc, char** argv)
{
    const std::string note = slurp(argc > 2 ? argv[1] : "design_note.txt");
    const std::string src = slurp(argc > 2 ? argv[2] : "edu4.cc");
    int problems = 0;

    const char* aspects[] = {"Bus binding", "Resources", "Initialization sequence", "Register model",
                             "Interrupts", "DMA and cache coherency", "Power states", "Error handling",
                             "Hot-plug and lifetime", "Upper interface", "Trust", "Observability"};
    for (const char* a : aspects) {
        const std::string head = std::string("## ") + a + "\n";
        const auto at = note.find(head);
        if (at == std::string::npos) {
            std::cout << "MISSING aspect: " << a << '\n';
            ++problems;
            continue;
        }
        const auto body_end = note.find("\n## ", at + head.size());
        const std::string body = note.substr(at + head.size(), body_end - at - head.size());
        const bool has_where = body.find("Where found:") != std::string::npos;
        std::cout << "aspect " << a << ": " << body.size() << " characters"
                  << (has_where ? ", has \"Where found\"" : ", NO \"Where found\" line") << '\n';
        problems += !has_where;
    }

    std::set<std::string> touched;
    const std::regex mmio(R"((?:rd|wr)\(d, (k\w+))");
    const std::regex cfg(R"(pci4::(?:read16|read32|write32)\(\s*f\.at, (PCI_\w+))");
    for (const std::regex* re : {&mmio, &cfg})
        for (std::sregex_iterator it(src.begin(), src.end(), *re), end; it != end; ++it) touched.insert((*it)[1]);
    if (src.find("pci4::read_bar") != std::string::npos) touched.insert("PCI_BASE_ADDRESS_0");

    std::map<std::string, std::string> table;
    std::istringstream lines(note);
    std::string line;
    while (std::getline(lines, line)) {
        std::smatch m;
        if (std::regex_match(line, m, std::regex(R"(REG (\w+) .*source: (.+))"))) table[m[1]] = m[2];
    }
    std::cout << "registers the driver touches: " << touched.size() << ", rows in the register table: "
              << table.size() << '\n';
    for (const auto& r : touched) {
        const auto it = table.find(r);
        if (it == table.end()) {
            std::cout << "  " << r << ": NOT IN THE DESIGN NOTE\n";
            ++problems;
        } else {
            std::cout << "  " << r << ": listed, source " << it->second.substr(0, 60)
                      << (it->second.size() > 60 ? "..." : "") << '\n';
        }
    }
    for (const auto& [r, s] : table)
        if (!touched.count(r)) std::cout << "  " << r << ": in the note but not touched by the driver\n";
    std::cout << (problems ? "FAIL: " : "PASS: ") << problems << " problem(s)\n";
    return problems ? 1 : 0;
}
