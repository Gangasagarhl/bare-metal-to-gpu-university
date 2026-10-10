// F11-15 Listing 3: print SHA-256 and HMAC-SHA-256 of fixed inputs, so that run.sh can
// compare them with Python's hashlib and hmac (an independent implementation).
#include <cstdio>

#include "hmac.h"

int main()
{
    const sec::Digest d1 = sec::sha256(sec::bytes("abc"));
    const sec::Digest d2 = sec::sha256(sec::bytes(std::string(1000, 'a')));
    const sec::Digest d3 = sec::hmacSha256(sec::bytes("robot-key-for-the-lab"),
                                           sec::bytes("seq=7;cmd=drive;v=0.30;w=0.00"));
    const sec::Digest d4 = sec::hmacSha256(sec::bytes(std::string(100, 'k')),
                                           sec::bytes("long key"));
    std::printf("sha256 abc        %s\n", sec::hex(d1.data(), d1.size()).c_str());
    std::printf("sha256 a*1000     %s\n", sec::hex(d2.data(), d2.size()).c_str());
    std::printf("hmac lab-command  %s\n", sec::hex(d3.data(), d3.size()).c_str());
    std::printf("hmac 100-byte-key %s\n", sec::hex(d4.data(), d4.size()).c_str());
    return 0;
}
