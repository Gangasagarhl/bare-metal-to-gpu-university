// hijack.cc - F11-07, demo 2: a memory bug becomes control-flow hijack.
// A record holds a name and, right after it, a pointer to the function that
// handles the next request. An over-long name overwrites that function pointer.
// Here the "attacker" supplies 16 filler bytes followed by the 8 bytes of the
// address of grant_root(): the overflow redirects the call. Normally the record
// would call handle_request(). Deterministic, no randomness.
//
// This is a teaching model of a real class of attack (overwriting a saved
// return address or a vtable/function pointer). Modern hardening (stack
// canaries, NX, ASLR, control-flow integrity) exists to break exactly this
// chain; those defences are chapter F11-10.
#include <cstdio>
#include <cstring>
#include <cstdint>

struct Request {
    char  name[16];
    void (*handler)();      // called after the name is set
};

static void handle_request() { std::printf("handle_request(): normal request handled\n"); }
static void grant_root()     { std::printf("grant_root(): *** attacker code ran ***\n"); }

// The bug: an unbounded copy into a 16-byte field.
static void fill_name(Request* r, const unsigned char* src, std::size_t n)
{
    std::memcpy(r->name, src, n);     // n can exceed 16 and reach r->handler
}

int main()
{
    Request r;
    r.handler = handle_request;

    // Craft 16 bytes of name + the 8-byte address of grant_root, as an attacker
    // who knows (or has leaked) that address would. On a 64-bit little-endian
    // machine the pointer is written byte for byte over r.handler.
    unsigned char payload[24];
    std::memset(payload, 'A', 16);
    std::uintptr_t target = reinterpret_cast<std::uintptr_t>(&grant_root);
    std::memcpy(payload + 16, &target, 8);

    std::printf("before: handler = %s\n",
                r.handler == handle_request ? "handle_request" : "something else");
    fill_name(&r, payload, sizeof payload);     // the overflow
    std::printf("after:  handler = %s\n",
                r.handler == grant_root ? "grant_root (hijacked!)" : "unchanged");

    r.handler();            // the program thinks it is handling a request
    return 0;
}
