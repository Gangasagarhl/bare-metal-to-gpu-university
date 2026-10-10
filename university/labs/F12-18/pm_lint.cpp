// pm_lint.cpp - F12-18 Listing 2: a checker for post-mortem drafts (Markdown-like text on
// stdin). It cannot judge whether a post-mortem is true or wise; it finds the mechanical
// problems a reviewer would otherwise spend time on: missing sections, blame and hindsight
// wording, timeline entries without a source, action items without owner, due date or type.
// Exit status 0 = no findings, 1 = findings.
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

int main()
{
    const std::vector<std::string> required = {
        "summary", "impact", "timeline", "root cause and contributing factors", "detection",
        "response and recovery", "what went well", "what went wrong", "where we got lucky",
        "action items", "lessons"};
    // Wording that judges a person or argues from hindsight instead of describing the system.
    const std::vector<std::string> blame = {
        "should have", "could have", "failed to", "forgot", "careless", "human error",
        "obviously", "if only", "negligent", "fault of", "blame", "did not bother", "just"};
    std::vector<std::string> seen;
    std::string section, line;
    int n = 0, findings = 0;
    auto report = [&](int at, const std::string& what) {
        std::printf("line %3d: %s\n", at, what.c_str());
        ++findings;
    };
    while (std::getline(std::cin, line)) {
        ++n;
        const std::string low = lower(line);
        if (low.rfind("## ", 0) == 0) {
            section = low.substr(3);
            seen.push_back(section);
            continue;
        }
        for (const std::string& b : blame) {
            const std::size_t p = low.find(b);
            // whole words only, so that "just" does not match "adjust"
            const bool word = p != std::string::npos &&
                              (p == 0 || !std::isalpha(static_cast<unsigned char>(low[p - 1]))) &&
                              (p + b.size() >= low.size() ||
                               !std::isalpha(static_cast<unsigned char>(low[p + b.size()])));
            if (word) {
                report(n, "blame or hindsight wording \"" + b +
                              "\": describe what the system allowed");
            }
        }
        if (low.rfind("- ", 0) != 0) {
            continue;
        }
        if (section == "timeline" && low.find('[') == std::string::npos) {
            report(n, "timeline entry without a [source] (alert, log, chat, metric, trace)");
        }
        if (section == "action items") {
            if (low.find("owner:") == std::string::npos) {
                report(n, "action item without owner:");
            }
            if (low.find("due:") == std::string::npos) {
                report(n, "action item without due:");
            }
            const bool typed = low.find("(prevent)") != std::string::npos ||
                               low.find("(detect)") != std::string::npos ||
                               low.find("(mitigate)") != std::string::npos ||
                               low.find("(process)") != std::string::npos;
            if (!typed) {
                report(n, "action item without a type: (prevent), (detect), (mitigate) or "
                          "(process)");
            }
        }
    }
    for (const std::string& r : required) {
        if (std::find(seen.begin(), seen.end(), r) == seen.end()) {
            report(0, "missing section \"## " + r + "\"");
        }
    }
    std::printf("%d finding(s) in %d lines\n", findings, n);
    return findings == 0 ? 0 : 1;
}
