// canary.cc - F11-10: the stack protector (stack canary). A function copies an
// unbounded string into a 16-byte stack buffer. When the string is too long it
// overwrites everything after the buffer, including the saved return address.
// The compiler option -fstack-protector places a secret "canary" value between
// the buffer and the return address and checks it before returning; if the
// overflow changed it, the program aborts with "stack smashing detected"
// instead of returning to an attacker-chosen address.
// Built by run.sh twice at -O0 (so _FORTIFY_SOURCE does not interfere): once
// with -fstack-protector-all, once with -fno-stack-protector.
#include <cstdio>
#include <cstring>

static void greet(const char* name)
{
    char buf[16];
    std::strcpy(buf, name);          // the bug: no bound
    std::printf("hello, %s\n", buf);
}

int main(int argc, char** argv)
{
    greet(argc > 1 ? argv[1] : "friend");
    std::printf("greet returned normally\n");
    return 0;
}
