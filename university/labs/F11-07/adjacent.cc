// adjacent.cc - F11-07, demo 1: a memory bug becomes a security decision.
// A fixed-size name buffer sits right before an "is_admin" flag inside one
// struct. An unbounded copy of an over-long name writes past the 16-byte name
// field and into the flag. The program never checked a password; the overflow
// alone flipped a privilege bit. This is the whole idea of the chapter in ten
// lines: the attacker does not need a logic bug, only a length bug.
//
// The copy length comes from argv so the run.sh can show both a safe name and a
// 24-byte attack string. Built BOTH ways by run.sh: plain -O0 (to watch the
// corruption happen) and with -fsanitize=address (to show, honestly, that ASan
// does NOT catch an overflow that stays inside one object).
#include <cstdio>
#include <cstring>

struct Account {
    char user[16];
    int  is_admin;      // immediately after the name: the overflow lands here
};

// The bug: copies all of src, however long, into a 16-byte field. A hand-written
// loop (not strcpy) so that no library hardening (_FORTIFY_SOURCE) steps in; this
// is the raw bug. It writes src's whole length, not 16 bytes.
static void set_name(Account& a, const char* src)
{
    std::size_t n = std::strlen(src);
    for (std::size_t i = 0; i < n; ++i) a.user[i] = src[i];   // no bound check
}

int main(int argc, char** argv)
{
    const char* name = (argc > 1) ? argv[1] : "amina";
    Account a;
    std::memset(&a, 0, sizeof a);
    a.is_admin = 0;                      // nobody is admin to begin with

    set_name(a, name);

    std::printf("user set to \"%.16s\"; is_admin = %d -> %s\n",
                a.user, a.is_admin, a.is_admin ? "ADMIN ACCESS GRANTED" : "normal user");
    return 0;
}
