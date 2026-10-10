// F9-33 forensic evidence generator: "The robot that thinks it is slow".
// The position sensor was replaced by one that reports 20 times per second instead of 10.
// The filter code of Listing 2 was not changed. This program runs that filter on the new
// recording and prints what the developer looked at.
#include "mat.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

int main()
{
    const double dt = 0.1;  // as in Listing 2
    const Mat F(2, 2, {1.0, dt, 0.0, 1.0});
    const Mat G(2, 1, {0.5 * dt * dt, dt});
    const Mat Q = 0.25 * (G * G.t());
    const Mat H(1, 2, {1.0, 0.0});
    const Mat R(1, 1, {0.01});
    const Mat I = Mat::identity(2);
    Mat x(2, 1, {0.0, 0.0});
    Mat P = Mat::identity(2);

    std::ifstream in("track_fast.csv");
    std::string line;
    std::getline(in, line);
    std::cout << "first lines of the recording (t, z, ...):\n";
    std::vector<double> innov, nis, vEst, vOdo, vVar;
    int k = 0;
    while (std::getline(in, line)) {
        if (k < 3) {
            std::cout << "  " << line << '\n';
        }
        std::istringstream f(line);
        double t = 0, z = 0, tp = 0, tv = 0;
        char c = ',';
        f >> t >> c >> z >> c >> tp >> c >> tv;
        ++k;
        x = F * x;
        P = F * P * F.t() + Q;
        const Mat y = Mat(1, 1, {z}) - H * x;
        const Mat S = H * P * H.t() + R;
        const Mat K = P * H.t() * inverse(S);
        x = x + K * y;
        const Mat IKH = I - K * H;
        P = IKH * P * IKH.t() + K * R * K.t();
        if (k > 40) {
            innov.push_back(y(0, 0));
            nis.push_back(y(0, 0) * y(0, 0) / S(0, 0));
            vEst.push_back(x(1, 0));
            vOdo.push_back(tv);  // wheel-odometry speed logged alongside (here: the true speed)
            vVar.push_back(P(1, 1));
        }
    }
    const double n = static_cast<double>(innov.size());
    double mNis = 0.0, mNu = 0.0, absEst = 0.0, absOdo = 0.0, vNorm = 0.0;
    for (std::size_t i = 0; i < innov.size(); ++i) {
        mNis += nis[i];
        mNu += innov[i];
        absEst += std::fabs(vEst[i]);
        absOdo += std::fabs(vOdo[i]);
        vNorm += (vEst[i] - vOdo[i]) * (vEst[i] - vOdo[i]) / vVar[i];
    }
    mNu /= n;
    double c0 = 0.0, c1 = 0.0, sxy = 0.0, sxx = 0.0;
    for (std::size_t i = 0; i < innov.size(); ++i) {
        c0 += (innov[i] - mNu) * (innov[i] - mNu);
        if (i > 0) {
            c1 += (innov[i] - mNu) * (innov[i - 1] - mNu);
        }
        sxy += vOdo[i] * vEst[i];
        sxx += vOdo[i] * vOdo[i];
    }
    std::cout << std::fixed << std::setprecision(4) << "samples analysed: " << innov.size() << '\n'
              << "mean NIS: " << mNis / n << '\n'
              << "lag-1 autocorrelation of innovations: " << c1 / c0 << '\n'
              << "mean |filter velocity|: " << absEst / n << " m/s, mean |odometry speed|: "
              << absOdo / n << " m/s\n"
              << "mean of (filter velocity - odometry speed)^2 / P_vv: " << vNorm / n << '\n'
              << "filter velocity / odometry speed (least-squares slope): " << sxy / sxx << '\n';
    return 0;
}
