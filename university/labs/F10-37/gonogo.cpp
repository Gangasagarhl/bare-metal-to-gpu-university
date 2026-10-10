// gonogo.cpp - field-test readiness review in the course's text format.
// Reads one readiness record (stdin) and prints GO or NO-GO with every reason.
// The limits it compares against are written INTO the record with their source document;
// this program contains no weather, battery or legal limit of its own.
#include <cstdio>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

std::vector<std::string> tokens(const std::string& line)  // words and "quoted strings"
{
    std::vector<std::string> t;
    std::size_t i = 0;
    while (i < line.size()) {
        if (line[i] == ' ' || line[i] == '\t') {
            ++i;
            continue;
        }
        if (line[i] == '"') {
            const std::size_t j = line.find('"', i + 1);
            t.push_back(line.substr(i + 1, j - i - 1));
            i = (j == std::string::npos) ? line.size() : j + 1;
        } else {
            std::size_t j = i;
            while (j < line.size() && line[j] != ' ' && line[j] != '\t') ++j;
            t.push_back(line.substr(i, j - i));
            i = j;
        }
    }
    return t;
}

std::string field(const std::vector<std::string>& t, const std::string& key)
{
    for (std::size_t i = 0; i + 1 < t.size(); ++i)
        if (t[i] == key) return t[i + 1];
    return "";
}

bool has(const std::vector<std::string>& t, const std::string& word)
{
    for (const auto& w : t)
        if (w == word) return true;
    return false;
}

long minutes(const std::string& iso)  // "YYYY-MM-DDTHH:MM" -> minutes since 1970-01-01
{
    if (iso.size() < 16) return -1;
    int y = std::stoi(iso.substr(0, 4));
    const int m = std::stoi(iso.substr(5, 2)), d = std::stoi(iso.substr(8, 2));
    y -= m <= 2;  // days_from_civil (proleptic Gregorian calendar)
    const long era = (y >= 0 ? y : y - 399) / 400;
    const long yoe = y - era * 400;
    const long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long days = era * 146097 + doe - 719468;
    return days * 1440 + std::stoi(iso.substr(11, 2)) * 60 + std::stoi(iso.substr(14, 2));
}

int main()
{
    std::vector<std::string> nogo, notes;
    std::string line, fw, cfg, vfw, vcfg, start_s;
    std::map<std::string, std::string> roles;
    bool signed_off = false;
    // freshness: how old a check may be when the test starts (minutes), by category
    const std::map<std::string, long> fresh = {
        {"field", 60}, {"day", 12 * 60}, {"doc", 365L * 1440}};
    std::vector<std::vector<std::string>> items;
    while (std::getline(std::cin, line)) {
        const auto t = tokens(line);
        if (t.empty() || t[0][0] == '#') continue;
        if (t[0] == "test") {
            start_s = field(t, "start");
            fw = field(t, "firmware");
            cfg = field(t, "config");
            std::printf("test: %s, start %s, firmware %s, config %s\n", t[1].c_str(),
                        start_s.c_str(), fw.c_str(), cfg.c_str());
        } else if (t[0] == "vehicle") {
            vfw = field(t, "firmware");
            vcfg = field(t, "config");
        } else if (t[0] == "role") {
            roles[t[1]] = t[2];
            if (t[1] == "supervisor") signed_off = field(t, "signed") == "yes";
        } else
            items.push_back(t);
    }
    const long start = minutes(start_s);
    std::printf("%-4s %-6s %-62s %-5s %8s\n", "id", "cat", "item", "state", "age");
    for (const auto& t : items) {
        if (t[0] == "item") {  // item ID CAT "text" status S checked TIME by NAME [hard]
            const std::string id = t[1], cat = t[2], status = field(t, "status");
            const long age = start - minutes(field(t, "checked"));
            const bool hard = has(t, "hard");
            std::printf("%-4s %-6s %-62s %-5s %6ld m\n", id.c_str(), cat.c_str(), t[3].c_str(),
                        status.c_str(), age);
            if (status != "ok" && hard) nogo.push_back(id + " (" + t[3] + "): " + status);
            if (status != "ok" && !hard) notes.push_back(id + ": " + status + " (not a stop item)");
            if (fresh.count(cat) && (age > fresh.at(cat) || age < 0))
                nogo.push_back(id + ": checked " + std::to_string(age) +
                               " min before start; limit for '" + cat + "' items is " +
                               std::to_string(fresh.at(cat)) + " min");
            if (field(t, "by").empty()) nogo.push_back(id + ": nobody signed this item");
        } else if (t[0] == "limit") {  // limit NAME max X measured Y unit U source "..."
            const double mx = std::stod(field(t, "max")), me = std::stod(field(t, "measured"));
            std::printf("     limit %-18s measured %5.1f, max %5.1f %s  (source: %s)\n",
                        t[1].c_str(), me, mx, field(t, "unit").c_str(), field(t, "source").c_str());
            if (field(t, "source").empty())
                nogo.push_back("limit " + t[1] + ": no source document for the limit");
            if (me > mx)
                nogo.push_back("limit " + t[1] + ": measured " + field(t, "measured") + " > max " +
                               field(t, "max"));
        }
    }
    if (vfw != fw || vcfg != cfg)
        nogo.push_back("vehicle reports firmware " + vfw + " config " + vcfg +
                       ", the record was prepared for " + fw + " " + cfg);
    for (const char* r : {"pilot", "observer", "supervisor"})
        if (!roles.count(r)) nogo.push_back(std::string("no ") + r + " named");
    if (roles.count("pilot") && roles.count("observer") && roles["pilot"] == roles["observer"])
        nogo.push_back("pilot and observer are the same person");
    if (!signed_off) nogo.push_back("supervisor has not signed");

    for (const auto& n : notes) std::printf("note: %s\n", n.c_str());
    std::printf("\n%s\n", nogo.empty() ? "GO" : "NO-GO");
    for (const auto& n : nogo) std::printf("  - %s\n", n.c_str());
    return nogo.empty() ? 0 : 1;
}
