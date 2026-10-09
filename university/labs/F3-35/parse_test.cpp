// parse_test.cpp - F3-35: what the minish parser makes of some command lines.
#include <cstdio>
#include "minish_parse.h"

int main()
{
    const char* lines[] = {
        "echo hello world",
        "sort < fruit.txt | head -n 2 > top.txt",
        "echo 'a  b' \"c $? d\" e\\ f >> log.txt",
        "false; echo status $?",
        "cat file | | wc",
        "echo > ",
        "echo 'unfinished",
        "# only a comment",
    };
    for (const char* l : lines) {
        std::printf("line: %s\n", l);
        std::vector<Token> toks;
        std::vector<Pipeline> pipes;
        std::string err;
        if (!tokenize(l, 1, toks, err) || !parse(toks, pipes, err)) { std::printf("  error: %s\n", err.c_str()); continue; }
        if (pipes.empty()) std::printf("  (nothing to run)\n");
        for (std::size_t p = 0; p < pipes.size(); ++p) {
            for (std::size_t c = 0; c < pipes[p].size(); ++c) {
                const Command& cmd = pipes[p][c];
                std::printf("  pipeline %zu, command %zu: argv =", p, c);
                for (const auto& a : cmd.argv) std::printf(" [%s]", a.c_str());
                if (!cmd.in.empty()) std::printf("  stdin < %s", cmd.in.c_str());
                if (!cmd.out.empty()) std::printf("  stdout %s %s", cmd.append ? ">>" : ">", cmd.out.c_str());
                std::printf("\n");
            }
        }
    }
    return 0;
}
