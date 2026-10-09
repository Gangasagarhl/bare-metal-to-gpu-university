// tty.cc - DR301 F4-03: canonical input. Characters collect in a buffer and are echoed;
// backspace removes the last one; Enter hands the finished line to the reader.
#include "tty.h"

bool LineDiscipline::feed(char c)
{
    if (c == '\b' || c == 0x7F) {                 // erase one character, also on the screen
        if (n_ > 0) { --n_; echo_('\b'); echo_(' '); echo_('\b'); }
        return false;
    }
    if (c == '\n' || c == '\r') {
        for (size_t i = 0; i < n_; ++i) done_[i] = buf_[i];
        done_[n_] = 0;
        n_ = 0;
        echo_('\n');
        return true;
    }
    if (c >= ' ' && n_ + 1 < sizeof(buf_)) {      // printable and room left
        buf_[n_++] = c;
        echo_(c);
    }
    return false;
}
