// minish.cc - F3-35: a small shell. It needs only these C library calls, which is why it is a good
// first program for a ported C library: fork, execvp, pipe, dup2, open, close, waitpid, chdir, _exit.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "minish_parse.h"

static int last_status = 0;

// In the child: connect stdin/stdout to files named in the command, if any.
static void redirect(const Command& c)
{
    if (!c.in.empty()) {
        int fd = open(c.in.c_str(), O_RDONLY);
        if (fd < 0) { std::fprintf(stderr, "minish: %s: %s\n", c.in.c_str(), std::strerror(errno)); _exit(1); }
        dup2(fd, 0);
        close(fd);
    }
    if (!c.out.empty()) {
        int fd = open(c.out.c_str(), O_WRONLY | O_CREAT | (c.append ? O_APPEND : O_TRUNC), 0644);
        if (fd < 0) { std::fprintf(stderr, "minish: %s: %s\n", c.out.c_str(), std::strerror(errno)); _exit(1); }
        dup2(fd, 1);
        close(fd);
    }
}

static bool builtin(const Command& c)                    // commands that must change the shell itself
{
    const std::string& name = c.argv[0];
    if (name == "cd") {
        const char* dir = c.argv.size() > 1 ? c.argv[1].c_str() : std::getenv("HOME");
        last_status = (dir && chdir(dir) == 0) ? 0 : 1;
        if (last_status) std::fprintf(stderr, "minish: cd: %s: %s\n", dir ? dir : "", std::strerror(errno));
        return true;
    }
    if (name == "exit") {
        std::exit(c.argv.size() > 1 ? std::atoi(c.argv[1].c_str()) : last_status);
    }
    return false;
}

static void run(const Pipeline& p)
{
    if (p.size() == 1 && builtin(p[0])) return;
    std::vector<pid_t> pids;
    int prev_read = -1;                                  // read end of the previous command's pipe
    for (std::size_t i = 0; i < p.size(); ++i) {
        int fds[2] = {-1, -1};
        bool last = i + 1 == p.size();
        if (!last && pipe(fds) != 0) { std::perror("minish: pipe"); return; }
        std::fflush(stdout);
        pid_t pid = fork();
        if (pid == 0) {                                  // child: wire up, then become the program
            if (prev_read >= 0) { dup2(prev_read, 0); close(prev_read); }
            if (!last) { dup2(fds[1], 1); close(fds[0]); close(fds[1]); }
            redirect(p[i]);
            std::vector<char*> argv;
            for (const auto& a : p[i].argv) argv.push_back(const_cast<char*>(a.c_str()));
            argv.push_back(nullptr);
            execvp(argv[0], argv.data());
            std::fprintf(stderr, "minish: %s: command not found\n", argv[0]);
            _exit(127);
        }
        if (prev_read >= 0) close(prev_read);            // the parent keeps no pipe ends open,
        if (!last) { close(fds[1]); prev_read = fds[0]; } // or readers would never see end of file
        pids.push_back(pid);
    }
    for (std::size_t i = 0; i < pids.size(); ++i) {      // the status of a pipeline is its last command's
        int st = 0;
        waitpid(pids[i], &st, 0);
        if (i + 1 == pids.size()) last_status = WIFEXITED(st) ? WEXITSTATUS(st) : 128 + WTERMSIG(st);
    }
}

int main()
{
    bool interactive = isatty(0);
    std::string line;
    while (true) {
        if (interactive) { std::printf("minish$ "); std::fflush(stdout); }
        if (!std::getline(std::cin, line)) break;
        std::vector<Token> toks;
        std::vector<Pipeline> pipes;
        std::string err;
        if (!tokenize(line, last_status, toks, err) || !parse(toks, pipes, err)) {
            std::fprintf(stderr, "minish: syntax error: %s\n", err.c_str());
            last_status = 2;
            continue;
        }
        for (const Pipeline& p : pipes) run(p);
    }
    return last_status;
}
