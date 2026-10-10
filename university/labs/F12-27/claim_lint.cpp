// claim_lint.cpp - find claims in a report that the evidence does not carry.
// Reads a report on standard input. Flags, per sentence:
//   P  padding that pretends to knowledge ("studies show", "industry standard", ...)
//   N  a number used as a claim (faster, %, times, ...) with no evidence tag [R..] or [D..]
//   A  an absolute word ("always", "never", "guaranteed", "zero risk", ...)
// The lists follow the university's own rules (guide AH-17 and 13.8) and are a
// starting point, not a complete list: a human still reads every sentence.
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::vector<std::string> sentences(const std::string& text)
{
    std::vector<std::string> out;
    std::string cur;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i] == '\n' ? ' ' : text[i];
        if (cur.empty() && c == ' ') {
            continue;
        }
        cur += c;
        const bool end = c == '.' || c == '!' || c == '?';
        const bool last = i + 1 == text.size();
        if (end && (last || std::isspace(static_cast<unsigned char>(text[i + 1])))) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (!cur.empty()) {
        out.push_back(cur);
    }
    return out;
}

bool containsAny(const std::string& s, const std::vector<std::string>& list, std::string& hit)
{
    for (const std::string& w : list) {
        if (s.find(w) != std::string::npos) {
            hit = w;
            return true;
        }
    }
    return false;
}

int main()
{
    const std::string text{std::istreambuf_iterator<char>(std::cin),
                           std::istreambuf_iterator<char>()};
    const std::vector<std::string> padding = {
        "studies show", "experts", "it is well known", "it is believed", "typically",
        "industry standard", "everyone knows", "best in class", "blazing", "revolutionary",
        "state of the art"};
    const std::vector<std::string> numberWords = {
        "faster", "slower", "%", "times", "×", "reduction", "improvement", "fewer", "more"};
    const std::vector<std::string> absolutes = {
        " always", " never", "guaranteed", "zero risk", "no risk", "cannot fail", "100 % safe"};

    const auto sents = sentences(text);
    int flagged = 0, counts[3] = {};
    for (std::size_t i = 0; i < sents.size(); ++i) {
        const std::string s = lower(" " + sents[i]);
        const bool hasDigit = std::any_of(s.begin(), s.end(),
                                          [](unsigned char c) { return std::isdigit(c); });
        const bool tagged = s.find("[r") != std::string::npos || s.find("[d") != std::string::npos;
        std::string hit, flags;
        if (containsAny(s, padding, hit)) {
            flags += " P(\"" + hit + "\")";
            ++counts[0];
        }
        if (hasDigit && containsAny(s, numberWords, hit) && !tagged) {
            flags += " N(\"" + hit + "\", no tag)";
            ++counts[1];
        }
        if (containsAny(s, absolutes, hit)) {
            flags += " A(\"" + hit.substr(hit[0] == ' ' ? 1 : 0) + "\")";
            ++counts[2];
        }
        if (!flags.empty()) {
            ++flagged;
            std::cout << "sentence " << i + 1 << ":" << flags << "\n    " << sents[i] << '\n';
        }
    }
    std::cout << "\nsentences: " << sents.size() << ", flagged: " << flagged << " (padding "
              << counts[0] << ", untagged numbers " << counts[1] << ", absolutes " << counts[2]
              << ")\n";
    return 0;
}
