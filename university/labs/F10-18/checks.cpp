// checks.cpp - recomputes the numbers used in F10-18's text, worked example and answer key.
#include <algorithm>
#include <cmath>
#include <cstdio>

int main()
{
    const double a = 0.15, c = 0.015, base = 9.81 / 4, top = 10.0;
    const double rp = 0.5 / (4 * a), y = 0.2 / (4 * c);
    const double sy[4] = {1, -1, -1, 1}, sz[4] = {1, -1, 1, -1};
    std::printf("W1 roll part %.4f N, yaw part %.4f N, base %.4f N\n", rp, y, base);
    double f[4];
    for (int i = 0; i < 4; ++i) f[i] = base + sy[i] * rp + sz[i] * y;
    std::printf("W2 exact solution: %.4f %.4f %.4f %.4f N\n", f[0], f[1], f[2], f[3]);
    const double s = (base - rp) / y; // motor 2 reaches zero: base - rp - s*y = 0
    std::printf("W3 RP>T>Y yaw fraction %.4f -> tz %.4f N m\n", s, 0.2 * s);
    for (int i = 0; i < 4; ++i) f[i] = base + sy[i] * rp + sz[i] * s * y;
    std::printf("W4 RP>T>Y forces: %.4f %.4f %.4f %.4f N\n", f[0], f[1], f[2], f[3]);
    const double lo = rp + y; // most negative part is -(rp + y)
    std::printf("W5 RP>Y>T: base raised to %.4f N, thrust %.3f N\n", std::max(base, lo),
                4 * std::max(base, lo));
    std::printf("W6 yaw authority at hover 4 c f_hover = %.4f N m; roll authority (any thrust) "
                "2 a fmax = %.2f N m\n",
                4 * c * base, 2 * a * top);
    const double rq = 1.0 / (4 * a);
    std::printf("Q2 tx 1.0 N m at hover: roll part %.4f N, forces %.4f and %.4f N (fits)\n", rq,
                base + rq, base - rq);
    std::printf("Q4 yaw authority at hover, thrust kept: %.4f N m\n", 4 * c * base);
    const double b15 = 1.5 * 9.81 / 4;
    std::printf("Q6 1.5 kg: base %.4f N, yaw authority with thrust kept %.4f N m\n", b15,
                4 * c * std::min(b15, top - b15));
    return 0;
}
