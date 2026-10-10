// codemap.cpp - a first map of a large Python code base, made in seconds.
// Reads two lines on stdin: the root folder, and one file (relative to the root) to outline.
// Prints: size, areas by lines, largest files, entry points, most-imported modules, outline.
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct FileInfo
{
    std::string rel;   // path relative to the root
    std::size_t lines = 0;
    bool entry = false;  // has a __main__ guard, or is a __main__.py
};

std::string firstName(const std::string& s)  // "http.server, os" -> "http"
{
    std::size_t end = s.find_first_of(" .,;");
    return s.substr(0, end);
}

std::string trimLeft(const std::string& s)
{
    std::size_t i = s.find_first_not_of(" \t");
    return i == std::string::npos ? std::string() : s.substr(i);
}

int main()
{
    std::string root;
    std::string focus;
    std::getline(std::cin, root);
    std::getline(std::cin, focus);

    std::vector<FileInfo> files;
    std::map<std::string, std::set<std::string>> importers;  // module -> files importing it
    for (const auto& e : fs::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || e.path().extension() != ".py") continue;
        std::string rel = fs::relative(e.path(), root).generic_string();
        if (rel.find("__pycache__") != std::string::npos) continue;
        FileInfo f{rel, 0, e.path().filename() == "__main__.py"};
        std::ifstream in(e.path());
        std::string line;
        while (std::getline(in, line)) {
            ++f.lines;
            std::string t = trimLeft(line);
            if (t.rfind("if __name__ == ", 0) == 0) f.entry = true;
            std::string mod;
            if (t.rfind("import ", 0) == 0) mod = firstName(t.substr(7));
            if (t.rfind("from ", 0) == 0) mod = firstName(t.substr(5));
            if (!mod.empty()) importers[mod].insert(rel);  // "from . import x" gives ""
        }
        files.push_back(f);
    }

    std::size_t total = 0;
    std::map<std::string, std::pair<std::size_t, std::size_t>> areas;  // area -> files, lines
    for (const auto& f : files) {
        total += f.lines;
        std::size_t slash = f.rel.find('/');
        std::string area = slash == std::string::npos ? "(top-level .py files)"
                                                      : f.rel.substr(0, slash) + "/";
        areas[area].first += 1;
        areas[area].second += f.lines;
    }
    std::cout << "== 1. Size: " << files.size() << " .py files, " << total << " lines under "
              << root << "\n";

    std::vector<std::pair<std::string, std::pair<std::size_t, std::size_t>>> byLines(
        areas.begin(), areas.end());
    std::sort(byLines.begin(), byLines.end(),
              [](const auto& a, const auto& b) { return a.second.second > b.second.second; });
    std::cout << "== 2. Areas by lines (top 12 of " << byLines.size() << ")\n";
    for (std::size_t i = 0; i < byLines.size() && i < 12; ++i) {
        const auto& [name, fl] = byLines[i];
        std::cout << std::setw(8) << fl.second << " lines " << std::setw(5) << std::fixed
                  << std::setprecision(1) << 100.0 * double(fl.second) / double(total) << " % "
                  << std::setw(4) << fl.first << " files  " << name << "\n";
    }

    std::sort(files.begin(), files.end(),
              [](const FileInfo& a, const FileInfo& b) { return a.lines > b.lines; });
    std::cout << "== 3. Largest files\n";
    for (std::size_t i = 0; i < files.size() && i < 8; ++i) {
        std::cout << std::setw(8) << files[i].lines << "  " << files[i].rel << "\n";
    }

    std::size_t entries = 0;
    std::vector<std::string> mains;
    for (const auto& f : files) {
        if (f.entry) ++entries;
        if (f.rel.size() > 12 && f.rel.ends_with("/__main__.py")) mains.push_back(f.rel);
    }
    std::sort(mains.begin(), mains.end());
    std::cout << "== 4. Entry points: " << entries << " files can be run directly; "
              << "packages with __main__.py:\n   ";
    for (const auto& m : mains) std::cout << " " << m.substr(0, m.find('/'));
    std::cout << "\n";

    std::vector<std::pair<std::size_t, std::string>> hubs;
    for (const auto& [mod, who] : importers) hubs.push_back({who.size(), mod});
    std::sort(hubs.begin(), hubs.end(), std::greater<>());
    std::cout << "== 5. Hubs: modules imported by the most files\n";
    for (std::size_t i = 0; i < hubs.size() && i < 10; ++i) {
        std::cout << std::setw(6) << hubs[i].first << " files import " << hubs[i].second << "\n";
    }

    std::cout << "== 6. Outline of " << focus << " (classes, methods, top-level functions)\n";
    std::ifstream in(fs::path(root) / focus);
    std::string line;
    std::size_t n = 0;
    std::string cls;
    std::size_t clsLine = 0;
    std::size_t methods = 0;
    auto flush = [&]() {
        if (!cls.empty()) {
            std::cout << "  line " << std::setw(5) << clsLine << "  class " << cls << "  (methods: "
                      << methods << ")\n";
        }
        cls.clear();
        methods = 0;
    };
    while (std::getline(in, line)) {
        ++n;
        if (line.rfind("class ", 0) == 0) {
            flush();
            cls = line.substr(6, line.find_first_of("(:") - 6);
            clsLine = n;
        } else if (line.rfind("def ", 0) == 0) {
            flush();
            std::cout << "  line " << std::setw(5) << n << "  def "
                      << line.substr(4, line.find('(') - 4) << "\n";
        } else if (line.rfind("    def ", 0) == 0 && !cls.empty()) {
            ++methods;
        }
    }
    flush();
    std::cout << "  (" << n << " lines in total)\n";
    return 0;
}
