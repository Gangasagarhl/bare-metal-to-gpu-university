// F10-12 forensic analysis: reads rc_link.csv (t_ms,heading_deg,rssi_dbm; one row per frame
// received, a frame expected every 20 ms) and reports gaps and signal strength by heading.
#include <algorithm>
#include <cstdio>
#include <vector>

struct Row
{
    int t;
    double heading, rssi;
};

int main()
{
    std::FILE* f = std::fopen("rc_link.csv", "r");
    if (f == nullptr) {
        std::printf("rc_link.csv not found: run link_log first\n");
        return 1;
    }
    char header[64];
    if (std::fgets(header, sizeof header, f) == nullptr) {
        std::fclose(f);
        return 1;
    }
    std::vector<Row> rows;
    Row r{};
    while (std::fscanf(f, "%d,%lf,%lf", &r.t, &r.heading, &r.rssi) == 3) {
        rows.push_back(r);
    }
    std::fclose(f);

    int lostSector[12] = {}, gotSector[12] = {};
    double rssiSector[12] = {};
    int longest = 0, longestAt = 0, over100 = 0, over300 = 0;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const int sec = static_cast<int>(rows[i].heading / 30.0) % 12;
        ++gotSector[sec];
        rssiSector[sec] += rows[i].rssi;
        if (i > 0) {
            const int gap = rows[i].t - rows[i - 1].t;
            lostSector[sec] += gap / 20 - 1;            // missing frames before this one
            if (gap > longest) {
                longest = gap;
                longestAt = rows[i - 1].t;
            }
            over100 += gap > 100 ? 1 : 0;
            over300 += gap > 300 ? 1 : 0;
        }
    }
    std::printf("%zu frames; longest gap %d ms after t = %d ms; gaps > 100 ms: %d; "
                "gaps > 300 ms: %d\n", rows.size(), longest, longestAt, over100, over300);
    std::printf("heading sector  frames  lost  lost %%  mean RSSI dBm\n");
    for (int k = 0; k < 12; ++k) {
        const int all = gotSector[k] + lostSector[k];
        std::printf("   %3d-%3d     %6d %5d %7.1f %14.1f\n", 30 * k, 30 * k + 30, gotSector[k],
                    lostSector[k], all ? 100.0 * lostSector[k] / all : 0.0,
                    gotSector[k] ? rssiSector[k] / gotSector[k] : 0.0);
    }
    return 0;
}
