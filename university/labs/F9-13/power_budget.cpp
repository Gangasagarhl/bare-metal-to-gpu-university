// F9-13 Listing 1: a power budget for a small robot. Reads one line per load:
//   name rail_V average_A peak_A via   (via = "batt" if fed straight from the battery,
//                                       or "reg" if fed through a regulator)
// and prints power per load, totals at the battery, and a runtime estimate.
// Every number in power_budget.in is a PRETEND value; a real budget takes each one
// from the part's datasheet or from your own measurement, and cites it.
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    const double battV = 12.0;        // nominal pack voltage, V
    const double capacityAh = 5.0;    // rated capacity, Ah
    const double usable = 0.80;       // fraction of the capacity the plan allows itself to use
    const double regEff = 0.85;       // regulator efficiency
    std::string name, via;
    double v = 0.0, avg = 0.0, peak = 0.0;
    double sumAvgW = 0.0, sumPeakW = 0.0;
    std::printf("%-16s %6s %8s %8s %4s %10s %10s\n", "load", "rail_V", "avg_A", "peak_A", "via",
                "avg_W_batt", "peak_W_batt");
    while (std::cin >> name >> v >> avg >> peak >> via) {
        const double eff = (via == "reg") ? regEff : 1.0;
        const double avgW = v * avg / eff;    // power drawn from the battery for this load
        const double peakW = v * peak / eff;
        sumAvgW += avgW;
        sumPeakW += peakW;
        std::printf("%-16s %6.1f %8.3f %8.3f %4s %10.2f %10.2f\n", name.c_str(), v, avg, peak,
                    via.c_str(), avgW, peakW);
    }
    const double avgA = sumAvgW / battV, peakA = sumPeakW / battV;
    std::printf("\ntotal from battery: average %.1f W = %.2f A, "
                "all peaks together %.1f W = %.2f A\n",
                sumAvgW, avgA, sumPeakW, peakA);
    std::printf("runtime estimate: %.1f Ah x %.2f usable / %.2f A = %.2f h = %.0f min\n",
                capacityAh,
                usable, avgA, capacityAh * usable / avgA, 60.0 * capacityAh * usable / avgA);
    return 0;
}
