// nvme_forensic.cpp - F1-50 forensic evidence generator: "reads time out after a few
// commands". The same model as Listing 1, with the host built from a buggy driver.
#include "nvme_model.h"

#include <cstdio>

int main()
{
    const int entries = 4;
    Controller ctrl(entries);
    Host buggy(ctrl, entries, false);
    int outstanding = 0;
    int cid = 1;
    for (int batch = 1; batch <= 2; ++batch) {
        std::printf("driver: batch %d\n", batch);
        for (int i = 0; i < 3; ++i) {
            if (buggy.submit(cid)) {
                ++cid;
                ++outstanding;
            }
        }
        buggy.ring();
        for (int attempt = 1; attempt <= 3 && outstanding > 0; ++attempt) {
            outstanding -= buggy.poll();
        }
        if (outstanding > 0) {
            std::printf("driver: TIMEOUT, %d command(s) never completed\n", outstanding);
        }
    }
    std::printf("CQ memory dump (slot: cid/phase):");
    for (std::size_t i = 0; i < ctrl.cqMemory().size(); ++i) {
        std::printf(" %zu:%d/%d", i, ctrl.cqMemory()[i].cid, ctrl.cqMemory()[i].phase);
    }
    std::printf("\n");
    return 0;
}
