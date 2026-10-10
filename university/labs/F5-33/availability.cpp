// availability.cpp - the reliability arithmetic of F5-33, computed instead of typed:
//  * availability of one part from its mean time between failures (MTBF) and mean time to
//    repair (MTTR): A = MTBF / (MTBF + MTTR);
//  * parts in series (all needed): multiply availabilities;
//  * redundant parts (k of n needed, independent failures): sum the binomial terms;
//  * expected downtime per year = (1 - A) * 8766 hours (365.25 days), printed in minutes.
// The MTBF/MTTR values are EXERCISE VALUES chosen for this chapter, not data of any product.
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

double part(double mtbf_h, double mttr_h)
{
    return mtbf_h / (mtbf_h + mttr_h);
}

double binom(int n, int k)
{
    double r = 1.0;
    for (int i = 1; i <= k; ++i) {
        r = r * (n - k + i) / i;
    }
    return r;
}

// probability that at least k of n identical independent parts are up
double k_of_n(int k, int n, double a)
{
    double sum = 0.0;
    for (int up = k; up <= n; ++up) {
        sum += binom(n, up) * std::pow(a, up) * std::pow(1.0 - a, n - up);
    }
    return sum;
}

void show(const std::string& what, double a)
{
    std::cout << std::left << std::setw(46) << what << std::right << std::fixed
              << std::setprecision(7) << a << "   expected downtime "
              << std::setprecision(1) << std::setw(7) << (1.0 - a) * 8766.0 * 60.0 << " min/year\n";
}

int main()
{
    // exercise values (hours)
    const double psu = part(100000.0, 24.0);     // a power supply, replaced within a day
    const double fan = part(50000.0, 24.0);      // a fan module
    const double board = part(200000.0, 72.0);   // the main board (spare shipped: three days)

    show("one power supply", psu);
    show("power: 1 of 2 supplies needed (1+1)", k_of_n(1, 2, psu));
    show("one fan", fan);
    show("cooling: 4 of 4 fans needed", k_of_n(4, 4, fan));
    show("cooling: 3 of 4 fans needed (N+1)", k_of_n(3, 4, fan));
    show("main board", board);
    const double no_redundancy = psu * k_of_n(4, 4, fan) * board;
    const double redundant = k_of_n(1, 2, psu) * k_of_n(3, 4, fan) * board;
    show("server, no redundancy (1 PSU, 4 of 4 fans)", no_redundancy);
    show("server, 1+1 PSU and N+1 fans", redundant);
    // a service on several such servers: needs 2 of 3 up
    show("service needing 2 of 3 redundant servers", k_of_n(2, 3, redundant));
    return 0;
}
