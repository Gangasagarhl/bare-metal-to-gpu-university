// capability.cpp - DS403 F5-45: Amoeba-style capabilities, as a model.
// A capability names an object on a server and the rights its holder has. The server keeps
// a secret random number per object; the check field proves the rights were not raised.
//   owner capability:      rights = all,  check = secret
//   restricted capability: rights = r,    check = f(secret XOR r)
// f must be a one-way function. Here f is a 64-bit mixing function (splitmix64's finaliser):
// fine for a model, NOT cryptographically one-way. Field sizes are ours, not Amoeba's.
#include <cstdint>
#include <cstdio>
#include <map>
#include <random>

namespace {
struct Cap { uint32_t object; uint8_t rights; uint64_t check; };
constexpr uint8_t R_READ = 1, R_WRITE = 2, R_DELETE = 4, R_ALL = 7;

uint64_t f(uint64_t x)
{
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27; x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

class Server {
public:
    explicit Server(uint64_t seed) : rng_(seed) {}
    Cap create()
    {
        uint32_t id = next_++;
        secret_[id] = rng_();
        return Cap{id, R_ALL, secret_[id]};
    }
    // Anyone may ask the server to make a weaker copy of a capability they hold.
    bool restrict(const Cap& c, uint8_t keep, Cap& out) const
    {
        if (!valid(c)) return false;
        uint8_t r = static_cast<uint8_t>(c.rights & keep);
        out = Cap{c.object, r, f(secret_.at(c.object) ^ r)};
        return true;
    }
    bool valid(const Cap& c) const
    {
        auto it = secret_.find(c.object);
        if (it == secret_.end()) return false;
        if (c.rights == R_ALL) return c.check == it->second;
        return c.check == f(it->second ^ c.rights);
    }
    const char* use(const Cap& c, uint8_t need) const
    {
        if (!valid(c)) return "REFUSED: check field does not match";
        if ((c.rights & need) != need) return "REFUSED: right not in capability";
        return "allowed";
    }

private:
    std::mt19937_64 rng_;
    std::map<uint32_t, uint64_t> secret_;
    uint32_t next_ = 1;
};

void show(const char* who, const Cap& c)
{
    std::printf("  %-22s object %u rights %c%c%c check %016llx\n", who, c.object,
                (c.rights & R_READ) ? 'r' : '-', (c.rights & R_WRITE) ? 'w' : '-',
                (c.rights & R_DELETE) ? 'd' : '-', static_cast<unsigned long long>(c.check));
}
}  // namespace

int main()
{
    Server files(2026);                                 // fixed seed: the run is repeatable
    Cap owner = files.create();
    Cap reader{};
    files.restrict(owner, R_READ, reader);
    show("owner:", owner);
    show("given to a reader:", reader);

    std::printf("reader reads:            %s\n", files.use(reader, R_READ));
    std::printf("reader writes:           %s\n", files.use(reader, R_WRITE));
    Cap forged = reader;
    forged.rights = R_READ | R_WRITE;                   // edit the bits, keep the check
    show("reader's forgery:", forged);
    std::printf("forgery writes:          %s\n", files.use(forged, R_WRITE));
    Cap guess = forged;
    int hits = 0;
    std::mt19937_64 attacker(7);
    for (int i = 0; i < 1000000; ++i) {                 // try a million random check fields
        guess.check = attacker();
        if (files.valid(guess)) ++hits;
    }
    std::printf("random check guesses that worked: %d of 1000000\n", hits);
    Cap again{};
    files.restrict(reader, R_ALL, again);               // cannot widen: rights & keep
    show("reader asks for all:", again);
    std::printf("owner deletes:           %s\n", files.use(owner, R_DELETE));
    return 0;
}
