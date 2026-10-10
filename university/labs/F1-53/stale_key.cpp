// stale_key.cpp - F1-53 forensic evidence generator: "remote access errors after the
// buffer-pool refactor". Prints the logs of both hosts in the toy model's own format.
#include "rdma_model.h"

#include <cstdio>
#include <cstring>

int main()
{
    toy::Host server("server");
    toy::Host client("client");
    std::memcpy(&client.memory[0], "result-0001", 11);

    std::printf("server log:\n");
    std::uint32_t key = server.registerRegion(64, 64, toy::kLocalWrite | toy::kRemoteWrite);
    std::printf("  t=0.0 registered result buffer 64..127, key 0x%x; key sent to client\n", key);
    std::printf("client log:\n");
    std::printf("  t=0.1 cached server key 0x%x\n", key);
    toy::post(client, server, 1, toy::Op::Write, 0, 11, 64, key);
    toy::drain(client);

    std::printf("server log:\n");
    server.deregister(key);
    const std::uint32_t newKey = server.registerRegion(64, 64, toy::kLocalWrite | toy::kRemoteWrite);
    std::printf("  t=5.0 buffer pool resized: deregistered key 0x%x, registered 64..127 again, key 0x%x\n",
                key, newKey);
    std::printf("  t=5.0 (new key not announced: announce_keys() only runs at connection set-up)\n");
    std::printf("client log:\n");
    for (std::uint64_t id = 2; id <= 4; ++id) {
        toy::post(client, server, id, toy::Op::Write, 0, 11, 64, key);
    }
    toy::drain(client);
    std::printf("server log:\n");
    std::printf("  t=6.0 result buffer still contains \"%s\" (no new results since t=5.0)\n",
                server.text(64, 11).c_str());
    return 0;
}
