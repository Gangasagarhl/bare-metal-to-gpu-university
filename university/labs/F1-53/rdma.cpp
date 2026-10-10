// rdma.cpp - F1-53 Listing 1: one-sided RDMA WRITE and READ, two-sided SEND/RECEIVE,
// and the checks a remote request must pass, in the university's toy model.
#include "rdma_model.h"

#include <cstdio>
#include <cstring>

int main()
{
    toy::Host a("A");
    toy::Host b("B");

    // A registers bytes 0..63 so that remote peers may write into them; the key is sent
    // to B beforehand over an ordinary connection (not shown).
    const std::uint32_t aWriteKey = a.registerRegion(0, 64, toy::kLocalWrite | toy::kRemoteWrite);
    std::memcpy(&b.memory[128], "hello from B", 12);

    std::printf("1. one-sided write into A's memory\n");
    toy::post(b, a, 1, toy::Op::Write, 128, 12, 16, aWriteKey);
    std::printf("  A memory[16..27] = \"%s\" (A posted nothing for this)\n", a.text(16, 12).c_str());
    toy::drain(b);
    toy::drain(a);

    std::printf("2. one-sided read, but the region does not allow remote reads\n");
    toy::post(b, a, 2, toy::Op::Read, 160, 12, 16, aWriteKey);
    toy::drain(b);

    std::printf("3. write past the end of the registered region\n");
    toy::post(b, a, 3, toy::Op::Write, 128, 12, 60, aWriteKey);
    toy::drain(b);

    std::printf("4. two-sided send: first without, then with a posted receive\n");
    toy::post(b, a, 4, toy::Op::Send, 128, 12, 0, 0);
    a.receives.push_back(200);
    toy::post(b, a, 5, toy::Op::Send, 128, 12, 0, 0);
    std::printf("  A memory[200..211] = \"%s\"\n", a.text(200, 12).c_str());
    toy::drain(b);
    toy::drain(a);
    return 0;
}
