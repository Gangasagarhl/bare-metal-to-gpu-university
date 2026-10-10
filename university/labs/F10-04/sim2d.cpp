// sim2d.cpp - a planar (2D) quadrotor seen from behind: sideways y (left +), height z (up +),
// roll angle phi. The two left motors act as one rotor, the two right motors as another.
// Exercise values of the course quad (F10-04); semi-implicit Euler steps of 1 ms.
#include <cmath>
#include <cstdio>
#include <numbers>

struct Planar
{
    double y = 0.0, z = 0.0, phi = 0.0;  // m, m, rad
    double vy = 0.0, vz = 0.0, p = 0.0;  // m/s, m/s, rad/s
};

int main()
{
    const double mass = 1.0, g = 9.81, arm = 0.15, Jx = 0.010;  // kg, m/s^2, m, kg m^2
    const double dt = 0.001;
    const double half = mass * g / 2.0;  // hover force of each side, N
    const double kick = 0.2;             // extra force used to start and stop the roll, N
    const double rad2deg = 180.0 / std::numbers::pi;
    Planar s;
    std::printf("   t(s)    y(m)    z(m)  roll(deg)  vy(m/s)  vz(m/s)\n");
    for (int k = 0; k <= 3000; ++k) {
        const double t = k * dt;
        if (k % 250 == 0) {
            std::printf("%7.2f %7.3f %7.3f %9.2f %8.3f %8.3f\n", t, s.y, s.z, s.phi * rad2deg,
                        s.vy, s.vz);
        }
        double left = half, right = half;
        if (k >= 1000 && k < 1200) {      // 1.0 s to 1.2 s
            left += kick;   // left side pushes harder: positive roll (right side goes down)
            right -= kick;
        } else if (k >= 1200 && k < 1400) { // 1.2 s to 1.4 s
            left -= kick;   // the opposite pulse stops the rotation
            right += kick;
        } else if (k >= 1400) {
            const double total = mass * g / std::cos(s.phi);  // hover thrust condition
            left = total / 2.0;
            right = total / 2.0;
        }
        const double thrust = left + right;
        const double torque = arm * (left - right);
        // Newton for the body, Euler for the rotation; thrust is along the body z axis,
        // which after a roll phi points along (y, z) = (-sin phi, cos phi).
        s.vy += dt * (-thrust * std::sin(s.phi) / mass);
        s.vz += dt * (thrust * std::cos(s.phi) / mass - g);
        s.p += dt * (torque / Jx);
        s.y += dt * s.vy;
        s.z += dt * s.vz;
        s.phi += dt * s.p;
    }
    std::printf("predicted sideways acceleration after 1.4 s: -g tan(phi) = %.3f m/s^2\n",
                -g * std::tan(s.phi));
    return 0;
}
