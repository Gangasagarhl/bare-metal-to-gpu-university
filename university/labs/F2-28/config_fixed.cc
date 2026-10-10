// config_fixed.cc: config.cc after the forensic lab. A missing key is reported, not
// dereferenced; the program exits with status 2 so a start-up script notices.
#include <cstdio>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

struct Setting
{
    std::string key;
    std::string value;
};

std::vector<Setting> load(std::istream& in)
{
    std::vector<Setting> settings;
    std::string line;
    while (std::getline(in, line)) {
        const auto eq = line.find('=');
        if (eq != std::string::npos) {
            settings.push_back({line.substr(0, eq), line.substr(eq + 1)});
        }
    }
    return settings;
}

std::optional<std::string> find(const std::vector<Setting>& settings, const std::string& key)
{
    for (const Setting& s : settings) {
        if (s.key == key) {
            return s.value;
        }
    }
    return std::nullopt;
}

int main()
{
    const std::vector<Setting> settings = load(std::cin);
    const auto port = find(settings, "port");
    if (!port) {
        std::fprintf(stderr, "config error: no 'port' setting (keys are case-sensitive)\n");
        return 2;
    }
    std::printf("listening on port %d\n", std::stoi(*port));
    return 0;
}
