// status_lint.cpp - check a status update written for people who are not engineers.
// Reads the text on standard input and reports: sentence lengths, whether the ask
// or decision comes in the first two sentences, engineering jargon, unexplained
// acronyms, vague phrases, and whether dates and ranges are given.
// The word lists are this course's own starting point (F12-26); extend them.
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Splits on '.', '!' or '?' followed by white space or the end, so "2.5" stays whole.
std::vector<std::string> sentences(const std::string& text)
{
    std::vector<std::string> out;
    std::string cur;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i] == '\n' ? ' ' : text[i];
        cur += c;
        const bool end = c == '.' || c == '!' || c == '?';
        const bool last = i + 1 == text.size();
        if (end && (last || std::isspace(static_cast<unsigned char>(text[i + 1])))) {
            out.push_back(cur);
            cur.clear();
        }
    }
    if (cur.find_first_not_of(' ') != std::string::npos) {
        out.push_back(cur);
    }
    for (std::string& s : out) {
        s.erase(0, s.find_first_not_of(' '));
    }
    return out;
}

std::vector<std::string> words(const std::string& s)
{
    std::istringstream in(s);
    return {std::istream_iterator<std::string>(in), std::istream_iterator<std::string>()};
}

std::string strip(const std::string& w)
{
    std::string out;
    for (char c : w) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-') {
            out += c;
        }
    }
    return out;
}

int main()
{
    const std::string text{std::istreambuf_iterator<char>(std::cin),
                           std::istreambuf_iterator<char>()};
    const std::vector<std::string> jargon = {
        "dma", "irq", "interrupt", "kernel", "mutex", "race", "p99", "latency", "raft",
        "quorum", "sitl", "qemu", "regression", "refactor", "api", "flaky", "heap",
        "segfault", "throughput", "backlog", "merge", "deadlock", "leader", "replica"};
    const std::vector<std::string> vague = {
        "on track", "mostly", "should be fine", "soon", "almost done", "nearly done",
        "minor issues", "basically", "hopefully", "a few", "some issues", "asap", "going well"};
    const std::vector<std::string> ask = {"decide", "decision", "approve", "choose", "need you",
                                          "please confirm", "we ask"};
    const std::vector<std::string> when = {
        "monday", "tuesday", "wednesday", "thursday", "friday", "saturday", "sunday", "week",
        "january", "february", "march", "april", "may ", "june", "july", "august",
        "september", "october", "november", "december"};

    const auto sents = sentences(text);
    if (sents.empty()) {
        std::cout << "empty text\n";
        return 2;
    }
    std::size_t totalWords = 0, longest = 0, longestAt = 0;
    int issues = 0;
    std::cout << "sentences: " << sents.size() << '\n';
    for (std::size_t i = 0; i < sents.size(); ++i) {
        const auto w = words(sents[i]);
        totalWords += w.size();
        if (w.size() > longest) {
            longest = w.size();
            longestAt = i;
        }
    }
    std::cout << std::fixed << std::setprecision(1);
    const double avg = static_cast<double>(totalWords) / static_cast<double>(sents.size());
    std::cout << "words: " << totalWords << ", average per sentence " << avg << ", longest "
              << longest << " (sentence " << longestAt + 1 << ")\n";
    if (avg > 20.0) {
        std::cout << "  ! average above 20 words: split sentences\n";
        ++issues;
    }

    bool askEarly = false;
    for (std::size_t i = 0; i < std::min<std::size_t>(2, sents.size()); ++i) {
        for (const std::string& a : ask) {
            askEarly = askEarly || lower(sents[i]).find(a) != std::string::npos;
        }
    }
    std::cout << "ask or decision in the first two sentences: " << (askEarly ? "yes" : "no")
              << '\n';
    if (!askEarly) {
        std::cout << "  ! the reader must search for what you need from them\n";
        ++issues;
    }

    for (std::size_t i = 0; i < sents.size(); ++i) {
        const auto w = words(sents[i]);
        const std::string ls = lower(sents[i]);
        for (std::size_t k = 0; k < w.size(); ++k) {
            const std::string bare = strip(w[k]);
            const std::string lw = lower(bare);
            if (std::find(jargon.begin(), jargon.end(), lw) != jargon.end()) {
                std::cout << "  ! jargon \"" << bare << "\" in sentence " << i + 1 << '\n';
                ++issues;
            }
            const bool caps = bare.size() >= 2 &&
                std::all_of(bare.begin(), bare.end(), [](unsigned char c) {
                    return std::isupper(c) || std::isdigit(c);
                }) && std::any_of(bare.begin(), bare.end(), [](unsigned char c) {
                    return std::isupper(c);
                });
            const bool explained = k + 1 < w.size() && w[k + 1][0] == '(';
            if (caps && !explained) {
                std::cout << "  ! acronym \"" << bare << "\" not explained in sentence " << i + 1
                          << '\n';
                ++issues;
            }
        }
        for (const std::string& v : vague) {
            if (ls.find(v) != std::string::npos) {
                std::cout << "  ! vague \"" << v << "\" in sentence " << i + 1 << '\n';
                ++issues;
            }
        }
    }

    const std::string lt = lower(text);
    bool hasDate = false;
    for (const std::string& m : when) {
        hasDate = hasDate || lt.find(m) != std::string::npos;
    }
    const bool hasRange = lt.find(" to ") != std::string::npos ||
                          lt.find("between") != std::string::npos ||
                          text.find("–") != std::string::npos;
    std::cout << "names a date or week: " << (hasDate ? "yes" : "no")
              << "; gives a range: " << (hasRange ? "yes" : "no") << '\n';
    issues += !hasDate;
    issues += !hasRange;
    std::cout << "issues: " << issues << '\n';
    return 0;
}
