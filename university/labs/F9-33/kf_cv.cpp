// F9-33 Listing 2: the Kalman filter in matrix form, for a constant-velocity model.
// State x = [position, velocity]; only the position is measured. The filter estimates the
// velocity it never sees, and we check both estimates and the claimed covariance against
// the simulator's ground truth.
#include "mat.hpp"

#include <fstream>
#include <sstream>
#include <string>

int main()
{
    const double dt = 0.1;
    const double accelSd = 0.5;  // process noise: matches the simulator setting
    const double posSd = 0.1;    // measurement noise: matches the simulator setting

    const Mat F(2, 2, {1.0, dt, 0.0, 1.0});
    const Mat G(2, 1, {0.5 * dt * dt, dt});
    const Mat Q = (accelSd * accelSd) * (G * G.t());
    const Mat H(1, 2, {1.0, 0.0});
    const Mat R(1, 1, {posSd * posSd});
    const Mat I = Mat::identity(2);

    Mat x(2, 1, {0.0, 0.0});
    Mat P(2, 2, {1.0, 0.0, 0.0, 1.0});
    print("F", F);
    print("Q", Q);

    std::ifstream in("track_log.csv");
    std::string line;
    std::getline(in, line);
    double seP = 0.0, seV = 0.0, seZ = 0.0, nees = 0.0, nis = 0.0;
    int n = 0, k = 0;
    while (std::getline(in, line)) {
        std::istringstream f(line);
        double t = 0, z = 0, tp = 0, tv = 0;
        char c = ',';
        f >> t >> c >> z >> c >> tp >> c >> tv;
        ++k;
        // Predict
        x = F * x;
        P = F * P * F.t() + Q;
        // Update
        const Mat zm(1, 1, {z});
        const Mat y = zm - H * x;                 // innovation
        const Mat S = H * P * H.t() + R;          // innovation covariance
        const Mat K = P * H.t() * inverse(S);     // Kalman gain
        x = x + K * y;
        const Mat IKH = I - K * H;
        P = IKH * P * IKH.t() + K * R * K.t();    // Joseph form: stays symmetric, positive
        if (k == 1 || k == 2 || k == 300) {
            std::cout << "\nstep " << k << " (t = " << std::setprecision(1) << t << " s)\n";
            print("K", K);
            print("x", x);
            print("P", P);
        }
        if (k > 20) {
            const Mat e(2, 1, {x(0, 0) - tp, x(1, 0) - tv});
            nees += (e.t() * inverse(P) * e)(0, 0);
            nis += (y.t() * inverse(S) * y)(0, 0);
            seP += e(0, 0) * e(0, 0);
            seV += e(1, 0) * e(1, 0);
            seZ += (z - tp) * (z - tp);
            ++n;
        }
    }
    std::cout << "\nover steps 21-300 (" << n << " steps):\n"
              << "  RMS position error: readings " << std::sqrt(seZ / n) << " m, filter "
              << std::sqrt(seP / n) << " m\n"
              << "  RMS velocity error (velocity is never measured): " << std::sqrt(seV / n)
              << " m/s\n"
              << "  claimed sd at the end: position " << std::sqrt(P(0, 0)) << " m, velocity "
              << std::sqrt(P(1, 1)) << " m/s\n"
              << "  mean NEES (2 states, consistent filter: about 2): " << nees / n << '\n'
              << "  mean NIS  (1 measurement, consistent filter: about 1): " << nis / n << '\n';
    return 0;
}
