// F9-34 Listing 2: the EKF's two Jacobians, derived by hand, checked against finite
// differences. g is the motion model, h the range-bearing measurement model.
#include "mat.hpp"

#include <functional>

const double kDt = 0.1;

Mat g(const Mat& s, double v, double w)  // motion: unicycle, one step of length dt
{
    return Mat(3, 1, {s(0, 0) + v * kDt * std::cos(s(2, 0)), s(1, 0) + v * kDt * std::sin(s(2, 0)),
                      s(2, 0) + w * kDt});
}

Mat G(const Mat& s, double v)  // dg/ds, by hand
{
    return Mat(3, 3, {1.0, 0.0, -v * kDt * std::sin(s(2, 0)),
                      0.0, 1.0, v * kDt * std::cos(s(2, 0)),
                      0.0, 0.0, 1.0});
}

Mat h(const Mat& s, double lx, double ly)  // measurement: range and bearing to a landmark
{
    const double dx = lx - s(0, 0), dy = ly - s(1, 0);
    return Mat(2, 1, {std::sqrt(dx * dx + dy * dy), std::atan2(dy, dx) - s(2, 0)});
}

Mat Hj(const Mat& s, double lx, double ly)  // dh/ds, by hand
{
    const double dx = lx - s(0, 0), dy = ly - s(1, 0);
    const double q = dx * dx + dy * dy;
    const double r = std::sqrt(q);
    return Mat(2, 3, {-dx / r, -dy / r, 0.0,
                      dy / q, -dx / q, -1.0});
}

// Central differences: column j = (f(s + e_j) - f(s - e_j)) / (2 eps).
Mat numeric(const std::function<Mat(const Mat&)>& f, const Mat& s, int outRows)
{
    const double eps = 1e-6;
    Mat J(outRows, 3);
    for (int j = 0; j < 3; ++j) {
        Mat plus = s, minus = s;
        plus(j, 0) += eps;
        minus(j, 0) -= eps;
        const Mat d = f(plus) - f(minus);
        for (int i = 0; i < outRows; ++i) {
            J(i, j) = d(i, 0) / (2.0 * eps);
        }
    }
    return J;
}

double maxAbsDiff(const Mat& a, const Mat& b)
{
    double m = 0.0;
    for (std::size_t i = 0; i < a.a.size(); ++i) {
        m = std::max(m, std::fabs(a.a[i] - b.a[i]));
    }
    return m;
}

int main()
{
    const Mat s(3, 1, {1.0, 2.0, 0.7});
    const double v = 0.5, lx = 4.0, ly = 4.0;
    const Mat Ga = G(s, v);
    const Mat Gn = numeric([&](const Mat& p) { return g(p, v, 0.2); }, s, 3);
    print("G by hand ", Ga);
    print("G numeric ", Gn);
    std::cout << "max |difference| = " << std::scientific << std::setprecision(2)
              << maxAbsDiff(Ga, Gn) << "\n\n";
    const Mat Ha = Hj(s, lx, ly);
    const Mat Hn = numeric([&](const Mat& p) { return h(p, lx, ly); }, s, 2);
    print("H by hand ", Ha);
    print("H numeric ", Hn);
    std::cout << "max |difference| = " << std::scientific << std::setprecision(2)
              << maxAbsDiff(Ha, Hn) << '\n';
    return 0;
}
