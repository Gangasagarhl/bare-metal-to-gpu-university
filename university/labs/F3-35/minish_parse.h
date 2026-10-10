// minish_parse.h - F3-35: the command-line language of minish, a deliberately small shell.
//   line     := pipeline { ";" pipeline }        "#" starts a comment
//   pipeline := command { "|" command }
//   command  := word { word | "<" word | ">" word | ">>" word }
// Words may use '...' (literal), "..." (with $? expanded) and \x outside quotes.
// Known difference from sh: $? is expanded when the whole line is read, not when each command
// runs, so "false; echo $?" prints the status of the previous line. Put such commands on two lines.
#pragma once
#include <string>
#include <vector>

struct Command
{
    std::vector<std::string> argv;
    std::string in, out;                 // redirection targets; empty = none
    bool append = false;                 // ">>" instead of ">"
};
using Pipeline = std::vector<Command>;

struct Token { enum Kind { Word, Pipe, In, Out, Append, Semi } kind; std::string text; };

// Split a line into tokens. last_status replaces "$?". Returns false and sets err on bad input.
inline bool tokenize(const std::string& s, int last_status, std::vector<Token>& out, std::string& err)
{
    std::size_t i = 0;
    while (i < s.size()) {
        char c = s[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { ++i; continue; }
        if (c == '#') break;
        if (c == '|') { out.push_back({Token::Pipe, "|"}); ++i; continue; }
        if (c == ';') { out.push_back({Token::Semi, ";"}); ++i; continue; }
        if (c == '<') { out.push_back({Token::In, "<"}); ++i; continue; }
        if (c == '>') {
            if (i + 1 < s.size() && s[i + 1] == '>') { out.push_back({Token::Append, ">>"}); i += 2; }
            else { out.push_back({Token::Out, ">"}); ++i; }
            continue;
        }
        std::string w;                                   // a word: runs until blank or operator
        while (i < s.size() && std::string(" \t\r\n|;<>").find(s[i]) == std::string::npos) {
            if (s[i] == '\'') {
                std::size_t end = s.find('\'', i + 1);
                if (end == std::string::npos) { err = "unterminated '"; return false; }
                w += s.substr(i + 1, end - i - 1);
                i = end + 1;
            } else if (s[i] == '"') {
                ++i;
                while (i < s.size() && s[i] != '"') {
                    if (s[i] == '\\' && i + 1 < s.size() && std::string("\"\\$").find(s[i + 1]) != std::string::npos) {
                        w += s[i + 1]; i += 2;
                    } else if (s[i] == '$' && i + 1 < s.size() && s[i + 1] == '?') {
                        w += std::to_string(last_status); i += 2;
                    } else {
                        w += s[i++];
                    }
                }
                if (i == s.size()) { err = "unterminated \""; return false; }
                ++i;
            } else if (s[i] == '\\' && i + 1 < s.size()) {
                w += s[i + 1]; i += 2;
            } else if (s[i] == '$' && i + 1 < s.size() && s[i + 1] == '?') {
                w += std::to_string(last_status); i += 2;
            } else {
                w += s[i++];
            }
        }
        out.push_back({Token::Word, w});
    }
    return true;
}

// Group tokens into pipelines of commands. Returns false and sets err on a syntax error.
inline bool parse(const std::vector<Token>& t, std::vector<Pipeline>& out, std::string& err)
{
    Pipeline p;
    Command c;
    auto end_command = [&]() -> bool {
        if (c.argv.empty()) { err = "empty command"; return false; }
        p.push_back(c);
        c = Command{};
        return true;
    };
    for (std::size_t i = 0; i < t.size(); ++i) {
        switch (t[i].kind) {
        case Token::Word: c.argv.push_back(t[i].text); break;
        case Token::In: case Token::Out: case Token::Append:
            if (i + 1 == t.size() || t[i + 1].kind != Token::Word) { err = "missing file name after " + t[i].text; return false; }
            if (t[i].kind == Token::In) c.in = t[i + 1].text;
            else { c.out = t[i + 1].text; c.append = t[i].kind == Token::Append; }
            ++i;
            break;
        case Token::Pipe:
            if (!end_command()) return false;
            break;
        case Token::Semi:
            if (c.argv.empty() && p.empty()) continue;   // empty statement: allowed
            if (!end_command()) return false;
            out.push_back(p);
            p.clear();
            break;
        }
    }
    if (!c.argv.empty() || !p.empty()) { if (!end_command()) return false; out.push_back(p); }
    return true;
}
