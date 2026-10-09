// config.cc: reads "key=value" lines from standard input and starts a server on the
// configured port. Bug report: "it crashes on the new machine; on mine it works".
#include <cstdio>
#include <iostream>
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

const Setting* find(const std::vector<Setting>& settings, const std::string& key)
{
    for (const Setting& s : settings) {
        if (s.key == key) {
            return &s;
        }
    }
    return nullptr;
}

int port_of(const std::vector<Setting>& settings)
{
    const Setting* port = find(settings, "port");
    return std::stoi(port->value);
}

int main()
{
    const std::vector<Setting> settings = load(std::cin);
    std::printf("read %zu settings\n", settings.size());
    std::fflush(stdout);
    std::printf("listening on port %d\n", port_of(settings));
    return 0;
}
