// failover.cpp - generates the evidence pack of the F5-12 forensic lab "tickets that were sold
// twice". A ticket counter is replicated primary-backup; the primary crashes; the backup takes
// over. The replication mode is part of the evidence (configuration excerpt); the cause is in
// the answer key. Clocks are synchronised in this scenario so that you can focus on replication.
#include <cstdio>
#include <string>
#include <vector>

std::string at(int ms)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "19:30:%02d.%03d", ms / 1000, ms % 1000);
    return buf;
}

int main()
{
    std::vector<std::string> client, primary, backup, monitor;
    int seq = 1040;              // last ticket number before the excerpt; both copies agree
    int backupSeq = 1040;
    std::vector<int> unshipped;
    int t = 0;
    for (int sale = 0; sale < 14; ++sale) {
        t += 180 + (sale * 37) % 90;
        ++seq;
        primary.push_back(at(t) + " primary: sold ticket " + std::to_string(seq) + " (seat "
                          + std::to_string(200 + seq % 97) + ")");
        client.push_back(at(t + 4) + " client: OK ticket " + std::to_string(seq));
        unshipped.push_back(seq);
        if (seq % 5 == 0) {      // changes are shipped to the backup every 5 sales
            primary.push_back(at(t + 6) + " primary: shipped tickets "
                              + std::to_string(unshipped.front())
                              + ".." + std::to_string(unshipped.back()) + " to backup");
            backup.push_back(at(t + 15) + " backup: applied tickets "
                             + std::to_string(unshipped.front())
                             + ".." + std::to_string(unshipped.back()));
            backupSeq = unshipped.back();
            unshipped.clear();
        }
    }
    const int crash = t + 70;
    primary.push_back(at(crash) + " primary: (log ends)");
    monitor.push_back(at(crash + 1500) + " monitor: primary missed heartbeats for 1500 ms -> DEAD");
    monitor.push_back(at(crash + 1503) + " monitor: promoting backup (last applied ticket "
                      + std::to_string(backupSeq) + ")");
    backup.push_back(at(crash + 1510) + " backup: promoted to primary; next ticket "
                     + std::to_string(backupSeq + 1));
    int nt = crash + 1700;
    for (int k = 1; k <= 3; ++k) {
        backup.push_back(at(nt) + " backup: sold ticket " + std::to_string(backupSeq + k)
                         + " (seat "
                         + std::to_string(200 + (backupSeq + k) % 97) + ")");
        client.push_back(at(nt + 4) + " client: OK ticket " + std::to_string(backupSeq + k));
        nt += 210;
    }
    auto dump = [](const char* name, const std::vector<std::string>& lines) {
        std::printf("=== %s ===\n", name);
        for (const std::string& l : lines) {
            std::printf("%s\n", l.c_str());
        }
        std::printf("\n");
    };
    dump("client.log (box office)", client);
    dump("primary.log", primary);
    dump("backup.log", backup);
    dump("monitor.log", monitor);
    std::printf("=== replication.conf (excerpt) ===\nmode = async\nship_every_n_changes = 5\n"
                "acknowledge_client_after = local_write\n");
    return 0;
}
