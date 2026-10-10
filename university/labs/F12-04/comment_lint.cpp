// comment_lint.cpp - SE301 F12-04: mechanical checks on code-review comments (stdin).
// A comment starts with a header line   @ <file>:<line> [<label>]
// followed by its text, up to the next header. Labels: blocking, suggestion, question,
// nit, praise. The checks are the course's rules for precise and kind comments:
//   - every comment has a location and a known label;
//   - a blocking comment says WHY (because / otherwise / so that / which means) and
//     proposes something or asks something (? / suggest / could / instead / add / try);
//   - no words that judge the person or pretend the point is self-evident.
// A tool cannot judge tone fully; a clean run means "no known problem", not "kind".
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Comment
{
    int line = 0;
    std::string where;
    std::string label;
    std::string text;
};

static std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool hasWord(const std::string& text, const std::string& w)
{
    const std::string t = " " + lower(text) + " ";
    for (std::size_t p = t.find(w); p != std::string::npos; p = t.find(w, p + 1)) {
        const bool before = !std::isalpha(static_cast<unsigned char>(t[p - 1]));
        const bool after = !std::isalpha(static_cast<unsigned char>(t[p + w.size()]));
        if (before && after) return true;
    }
    return false;
}

static bool hasAny(const std::string& text, const std::vector<std::string>& ws)
{
    return std::any_of(ws.begin(), ws.end(), [&](const auto& w) { return hasWord(text, w); });
}

int main()
{
    const std::vector<std::string> labels = {"blocking", "suggestion", "question", "nit", "praise"};
    const std::vector<std::string> why = {"because", "otherwise", "so that", "which means"};
    const std::vector<std::string> offer = {"suggest", "could", "instead", "add", "try", "would"};
    const std::vector<std::string> harsh = {"obviously", "just", "simply", "clearly", "lazy",
        "sloppy", "stupid", "why didn't you", "you always", "you never", "terrible", "wtf"};

    std::vector<Comment> cs;
    std::string line;
    int n = 0;
    while (std::getline(std::cin, line)) {
        ++n;
        if (line.rfind("@", 0) == 0) {
            Comment c;
            c.line = n;
            const auto open = line.find('[');
            const auto close = line.find(']');
            c.where = line.substr(1, open == std::string::npos ? std::string::npos : open - 1);
            c.where.erase(0, c.where.find_first_not_of(' '));
            c.where.erase(c.where.find_last_not_of(' ') + 1);
            if (open != std::string::npos && close != std::string::npos && close > open) {
                c.label = lower(line.substr(open + 1, close - open - 1));
            }
            cs.push_back(c);
        } else if (!cs.empty()) {
            cs.back().text += line + " ";
        }
    }

    int findings = 0;
    std::map<std::string, int> count;
    auto flag = [&findings](const Comment& c, const char* msg) {
        std::printf("comment at line %2d (%s): %s\n", c.line, c.where.c_str(), msg);
        ++findings;
    };
    for (const auto& c : cs) {
        ++count[c.label.empty() ? "(none)" : c.label];
        if (c.where.find(':') == std::string::npos) flag(c, "no file:line location");
        if (std::find(labels.begin(), labels.end(), c.label) == labels.end()) {
            flag(c, "no known label (blocking, suggestion, question, nit, praise)");
        }
        if (c.label == "blocking" && !hasAny(c.text, why)) {
            flag(c, "blocking comment does not say why");
        }
        const bool asks = c.text.find('?') != std::string::npos;
        if (c.label == "blocking" && !asks && !hasAny(c.text, offer)) {
            flag(c, "blocking comment neither proposes a change nor asks a question");
        }
        for (const auto& h : harsh) {
            if (hasWord(c.text, h)) {
                const std::string msg = "word \"" + h + "\" judges the person or hides the reason";
                flag(c, msg.c_str());
            }
        }
    }
    std::printf("%zu comments:", cs.size());
    for (const auto& [label, k] : count) std::printf(" %s %d", label.c_str(), k);
    std::printf("; %d findings\n", findings);
    return findings == 0 ? 0 : 1;
}
