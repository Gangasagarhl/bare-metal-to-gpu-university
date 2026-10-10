// F10-08 forensic analysis: reads compass_log.csv and, for each compass, prints the mean
// heading per current step and the least-squares slope of heading against current.
#include <cstdio>
#include <vector>

struct Row
{
    double t, amps, hInt, hExt;
};

void fit(const char* name, const std::vector<Row>& rows, bool internal)
{
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (const auto& r : rows) {
        const double y = internal ? r.hInt : r.hExt;
        sx += r.amps;
        sy += y;
        sxx += r.amps * r.amps;
        sxy += r.amps * y;
    }
    const double n = static_cast<double>(rows.size());
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    const double icept = (sy - slope * sx) / n;
    std::printf("%-9s heading = %.2f deg %+.3f deg per A x current\n", name, icept, slope);
}

int main()
{
    std::FILE* f = std::fopen("compass_log.csv", "r");
    if (f == nullptr) {
        std::printf("compass_log.csv not found: run bench_log first\n");
        return 1;
    }
    char header[128];
    if (std::fgets(header, sizeof header, f) == nullptr) {
        std::fclose(f);
        return 1;
    }
    std::vector<Row> rows;
    Row r{};
    while (std::fscanf(f, "%lf,%lf,%lf,%lf", &r.t, &r.amps, &r.hInt, &r.hExt) == 4) {
        rows.push_back(r);
    }
    std::fclose(f);
    std::printf("%zu rows\nstep  time s   mean A  internal deg  external deg\n", rows.size());
    for (int step = 0; step < 5; ++step) {
        double a = 0, hi = 0, he = 0;
        int n = 0;
        for (const auto& row : rows) {
            if (static_cast<int>(row.t / 5.0) == step) {
                a += row.amps;
                hi += row.hInt;
                he += row.hExt;
                ++n;
            }
        }
        std::printf("%4d  %2d-%2d  %7.2f  %12.2f  %12.2f\n", step, 5 * step, 5 * step + 5, a / n,
                    hi / n, he / n);
    }
    fit("internal", rows, true);
    fit("external", rows, false);
    return 0;
}
