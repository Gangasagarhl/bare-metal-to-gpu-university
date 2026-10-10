// F9-27 Listing 2: check the analytic Jacobian against finite differences, watch the
// determinant go to zero, and map a tool force to joint torques with J^T.
#include "jac.hpp"
#include <algorithm>
#include <cstdio>

int main()
{
    // 1. analytic Jacobian versus central finite differences at many configurations
    double worst = 0;
    for (int a = -180; a <= 180; a += 15) {
        for (int b = -180; b <= 180; b += 15) {
            const V2 q{a * std::numbers::pi / 180, b * std::numbers::pi / 180};
            const M2 J = jacobian(q);
            const double h = 1e-6;
            for (int j = 0; j < 2; ++j) {
                V2 qp = q, qm = q;
                qp[j] += h;
                qm[j] -= h;
                const V2 fp = fk(qp), fm = fk(qm);
                for (int i = 0; i < 2; ++i)
                    worst = std::max(worst, std::abs(J[i][j] - (fp[i] - fm[i]) / (2 * h)));
            }
        }
    }
    std::printf("analytic J vs finite differences, 625 configurations: max difference %.2e\n",
                worst);

    // 2. determinant and the joint speeds needed for a 0.1 m/s tool speed along x
    std::printf("\nt2 (deg)  det J (m^2)   L1*L2*sin(t2)   qdot for v = (0.1, 0) m/s (deg/s)\n");
    for (double t2deg : {90.0, 60.0, 30.0, 10.0, 3.0, 1.0}) {
        const V2 q{-t2deg * std::numbers::pi / 360,
                   t2deg * std::numbers::pi / 180}; // tool near the x axis
        const M2 J = jacobian(q);
        const V2 qd = solve(J, {0.1, 0.0});
        std::printf("%7.1f   %10.6f    %10.6f      (%9.2f, %9.2f)\n", t2deg, det(J),
                    L1 * L2 * std::sin(q[1]), deg(qd[0]), deg(qd[1]));
    }

    // 3. statics: torques that hold a 10 N downward force at the tool, tau = J^T F
    const V2 q{30 * std::numbers::pi / 180, 45 * std::numbers::pi / 180};
    const V2 tau = mul(transpose(jacobian(q)), {0.0, -10.0});
    const V2 p = fk(q);
    std::printf("\npose (30, 45) deg: tool at (%.4f, %.4f) m; holding F = (0, -10) N needs\n", p[0],
                p[1]);
    std::printf("tau1 = %.4f N m, tau2 = %.4f N m (sign: the joints must push against the load)\n",
                tau[0], tau[1]);
    return 0;
}
