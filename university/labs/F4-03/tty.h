// tty.h - DR301 F4-03: a minimal line discipline (canonical mode with line editing).
#pragma once
#include <stddef.h>

class LineDiscipline {
public:
    using Echo = void (*)(char);
    explicit LineDiscipline(Echo echo) : echo_(echo) {}
    // Feed one character; returns true when a complete line is ready in line().
    bool feed(char c);
    const char* line() const { return done_; }

private:
    Echo echo_;
    char buf_[128] = {};
    size_t n_ = 0;
    char done_[128] = {};
};
