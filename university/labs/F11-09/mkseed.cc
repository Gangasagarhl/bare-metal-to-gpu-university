// mkseed.cc - write one valid MAVLink-v1-style seed frame to a file, so the
// fuzzer starts from a structurally correct input and its mutations explore the
// parser instead of being rejected at the first byte. Usage: ./mkseed <out>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    std::vector<unsigned char> f;
    const unsigned len = 4;                 // payload length
    f.push_back(0xFE);                       // STX
    f.push_back(static_cast<unsigned char>(len));
    f.push_back(0);                          // seq
    f.push_back(1);                          // sysid
    f.push_back(1);                          // compid
    f.push_back(0);                          // msgid (HEARTBEAT in MAVLink; number not relied on)
    for (unsigned i = 0; i < len; ++i) f.push_back(static_cast<unsigned char>(i));
    f.push_back(0x11);                        // ck_a (toy)
    f.push_back(0x22);                        // ck_b (toy)
    FILE* o = std::fopen(argv[1], "wb");
    if (!o) return 2;
    std::fwrite(f.data(), 1, f.size(), o);
    std::fclose(o);
    return 0;
}
