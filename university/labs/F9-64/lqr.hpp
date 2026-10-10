// lqr.hpp - discrete-time linear-quadratic regulator by Riccati iteration (F9-62).
#pragma once
#include <stdexcept>
#include "lin.hpp"

struct LqrResult
{
    Mat K;      // optimal gain: u[k] = -K x[k]
    Mat P;      // cost-to-go matrix: J*(x0) = x0^T P x0
    int iters;  // Riccati iterations until convergence
};

// Minimise  sum_k ( x^T Q x + u^T R u )  subject to  x[k+1] = Ad x[k] + Bd u[k].
inline LqrResult dlqr(const Mat& Ad, const Mat& Bd, const Mat& Q, const Mat& R)
{
    Mat P = Q;
    for (int it = 1; it <= 100000; ++it) {
        const Mat BtP = tr(Bd) * P;
        const Mat K = inv(R + BtP * Bd) * (BtP * Ad);
        const Mat Pn = Q + tr(Ad) * P * Ad - tr(Ad) * P * Bd * K;
        const double change = maxAbs(Pn - P);
        P = 0.5 * (Pn + tr(Pn));  // keep P exactly symmetric
        if (change < 1e-10 * std::max(1.0, maxAbs(P))) {
            const Mat Bt = tr(Bd) * P;
            return {inv(R + Bt * Bd) * (Bt * Ad), P, it};
        }
    }
    throw std::runtime_error("dlqr: Riccati iteration did not converge");
}

inline Mat diag(std::initializer_list<double> d)
{
    Mat m(d.size(), d.size());
    std::size_t i = 0;
    for (double v : d) {
        m(i, i) = v;
        ++i;
    }
    return m;
}
