// dashboard.cpp - writes a static HTML SLO report for the month of slo_model.hpp to standard
// output (F5-42, Listing 3): a summary, a bar chart of the error budget each day consumed,
// drawn as inline SVG, and a table per day. No JavaScript: a report that any browser opens
// from a file and that can be archived with the incident review.
#include "slo_model.hpp"

#include <cstdio>
#include <vector>

constexpr double kSlo = 0.999;

int main()
{
    const auto month = generateMonth(labIncidents(), 42);
    std::vector<long> req(kDays, 0);
    std::vector<long> err(kDays, 0);
    long total = 0;
    long failed = 0;
    for (int m = 0; m < kMonth; ++m) {
        req[static_cast<std::size_t>(m / kMinutesPerDay)] += month[static_cast<std::size_t>(m)].requests;
        err[static_cast<std::size_t>(m / kMinutesPerDay)] += month[static_cast<std::size_t>(m)].errors;
        total += month[static_cast<std::size_t>(m)].requests;
        failed += month[static_cast<std::size_t>(m)].errors;
    }
    const double budget = (1.0 - kSlo) * static_cast<double>(total);

    std::printf("<!DOCTYPE html>\n<html lang=\"en\"><head><meta charset=\"utf-8\">\n");
    std::printf("<title>SLO report: key-value service, 30 days</title>\n");
    std::printf("<style>body{font-family:sans-serif;margin:16px;max-width:900px}"
                "td,th{padding:2px 8px;text-align:right}.bad{font-weight:bold}</style>\n");
    std::printf("</head><body>\n<h1>SLO report: key-value service, 30 days</h1>\n");
    std::printf("<p>Objective: %.1f %% of requests succeed. Measured: %.4f %% (%ld of %ld failed).\n",
                100 * kSlo, 100.0 * (1.0 - static_cast<double>(failed) / static_cast<double>(total)),
                failed, total);
    std::printf("Error budget: %.0f failed requests; used %.1f %%.</p>\n", budget,
                100.0 * static_cast<double>(failed) / budget);

    // bar chart: budget consumed per day, as a percentage of the month's budget
    const int w = 24;
    const double scale = 6.0;   // pixels per percent
    std::printf("<svg width=\"%d\" height=\"230\" role=\"img\" aria-label=\"Error budget consumed per day\">\n",
                kDays * w + 50);
    std::printf("<line x1=\"40\" y1=\"200\" x2=\"%d\" y2=\"200\" stroke=\"gray\"/>\n", kDays * w + 45);
    std::printf("<text x=\"0\" y=\"14\" font-size=\"11\">%% of monthly budget used per day</text>\n");
    for (int d = 0; d < kDays; ++d) {
        const double pct = 100.0 * static_cast<double>(err[static_cast<std::size_t>(d)]) / budget;
        const double h = pct * scale > 170 ? 170 : pct * scale;
        std::printf("<rect x=\"%d\" y=\"%.1f\" width=\"%d\" height=\"%.1f\" fill=\"%s\"><title>day %d: %.1f %%</title></rect>\n",
                    45 + d * w, 200 - h, w - 4, h, pct > 100.0 / kDays ? "firebrick" : "steelblue", d + 1, pct);
        if (d % 5 == 0) {
            std::printf("<text x=\"%d\" y=\"215\" font-size=\"11\">%d</text>\n", 45 + d * w, d + 1);
        }
    }
    std::printf("<text x=\"45\" y=\"228\" font-size=\"11\">day of the month; red bars used more than an even "
                "share (%.2f %%)</text>\n</svg>\n", 100.0 / kDays);

    std::printf("<table>\n<tr><th>day</th><th>requests</th><th>failed</th><th>SLI %%</th>"
                "<th>budget used %%</th><th>budget left %%</th></tr>\n");
    double left = 100.0;
    for (int d = 0; d < kDays; ++d) {
        const auto r = req[static_cast<std::size_t>(d)];
        const auto e = err[static_cast<std::size_t>(d)];
        const double pct = 100.0 * static_cast<double>(e) / budget;
        left -= pct;
        std::printf("<tr%s><td>%d</td><td>%ld</td><td>%ld</td><td>%.4f</td><td>%.1f</td><td>%.1f</td></tr>\n",
                    pct > 100.0 / kDays ? " class=\"bad\"" : "", d + 1, r, e,
                    100.0 * (1.0 - static_cast<double>(e) / static_cast<double>(r)), pct, left);
    }
    std::printf("</table>\n</body></html>\n");
    return 0;
}
