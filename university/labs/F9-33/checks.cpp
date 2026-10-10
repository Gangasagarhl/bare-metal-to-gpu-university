// F9-33 checks: the worked example (dt = 1 s) step by step, and the numbers quoted in the text.
#include "mat.hpp"

int main()
{
    const double dt = 1.0;
    const Mat F(2, 2, {1.0, dt, 0.0, 1.0});
    const Mat G(2, 1, {0.5 * dt * dt, dt});
    const Mat Q = 0.25 * (G * G.t());
    const Mat H(1, 2, {1.0, 0.0});
    const Mat R(1, 1, {0.25});
    Mat x(2, 1, {0.0, 1.0});
    Mat P = Mat::identity(2);
    print("Q", Q);
    x = F * x;
    print("F P F^T", F * P * F.t());
    P = F * P * F.t() + Q;
    print("x_pred", x);
    print("P_pred", P);
    const Mat y = Mat(1, 1, {1.4}) - H * x;
    const Mat S = H * P * H.t() + R;
    const Mat K = P * H.t() * inverse(S);
    print("y", y);
    print("S", S);
    print("K", K);
    x = x + K * y;
    P = (Mat::identity(2) - K * H) * P;
    print("x_new", x);
    print("P_new (simple form)", P);
    std::cout << "sd of position " << std::sqrt(P(0, 0)) << ", sd of velocity "
              << std::sqrt(P(1, 1)) << '\n';
    // Lab Q at dt = 0.1 s, accel sd 0.5 m/s^2.
    const Mat G1(2, 1, {0.005, 0.1});
    print("lab Q", 0.25 * (G1 * G1.t()));
    // Chi-square with 2 degrees of freedom: P(X > c) = exp(-c/2); 95 % bound.
    std::cout << "95 % bound of a chi-square with 2 dof: " << -2.0 * std::log(0.05) << '\n';
    // Joseph and simple forms agree for the optimal gain; the Joseph form costs more.
    return 0;
}
