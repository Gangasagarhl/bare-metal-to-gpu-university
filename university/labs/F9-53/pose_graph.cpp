// pose_graph.cpp - F9-53: what loop closures do to a trajectory.
// Nodes: the robot's poses at the 97 scans of the route, first guess from odometry
// (same noise and seeds as run A of slam_lite.cpp). Edges: the odometry motion between
// consecutive nodes, plus loop closures between nodes that are close together but far
// apart in time. Solved with Gauss-Newton on (x, y, heading), dense, numeric Jacobians.
// Simplification: the loop-closure measurements come from the simulator's true poses plus
// noise, standing in for a scan matcher; real systems must FIND them and can be wrong.
#include "slam.hpp"
#include <cstdio>

struct Edge { int i, j; rb::Delta meas; double wPos, wTh; };

// Solve A x = b for a symmetric positive definite A (dense Cholesky).
std::vector<double> solveSpd(std::vector<double> a, std::vector<double> b, int n)
{
    for (int j = 0; j < n; ++j) {
        double d = a[j * n + j];
        for (int k = 0; k < j; ++k) { d -= a[j * n + k] * a[j * n + k]; }
        a[j * n + j] = std::sqrt(d);
        for (int i = j + 1; i < n; ++i) {
            double s = a[i * n + j];
            for (int k = 0; k < j; ++k) { s -= a[i * n + k] * a[j * n + k]; }
            a[i * n + j] = s / a[j * n + j];
        }
    }
    for (int i = 0; i < n; ++i) {
        for (int k = 0; k < i; ++k) { b[i] -= a[i * n + k] * b[k]; }
        b[i] /= a[i * n + i];
    }
    for (int i = n - 1; i >= 0; --i) {
        for (int k = i + 1; k < n; ++k) { b[i] -= a[k * n + i] * b[k]; }
        b[i] /= a[i * n + i];
    }
    return b;
}

// Residual of one edge: (predicted motion from node i to node j) - (measured motion).
void residual(const rb::Pose& a, const rb::Pose& b, const rb::Delta& m, double r[3])
{
    const rb::Delta d = rb::between(a, b);
    r[0] = d.fwd - m.fwd;
    r[1] = d.left - m.left;
    r[2] = rb::wrapAngle(d.dth - m.dth);
}

double optimise(std::vector<rb::Pose>& x, const std::vector<Edge>& edges, int iterations)
{
    const int n = 3 * static_cast<int>(x.size());
    double chi2 = 0.0;
    for (int it = 0; it < iterations; ++it) {
        std::vector<double> h(n * n, 0.0);
        std::vector<double> g(n, 0.0);
        for (int d = 0; d < 3; ++d) { h[d * n + d] += 1e9; }   // node 0 is held fixed
        chi2 = 0.0;
        for (const Edge& e : edges) {
            double r[3];
            residual(x[e.i], x[e.j], e.meas, r);
            const double w[3] = {e.wPos, e.wPos, e.wTh};
            for (int d = 0; d < 3; ++d) { chi2 += w[d] * r[d] * r[d]; }
            double jac[3][6];                                     // numeric derivatives
            for (int v = 0; v < 6; ++v) {
                rb::Pose a = x[e.i];
                rb::Pose b = x[e.j];
                double* p = v < 3 ? (v == 0 ? &a.x : v == 1 ? &a.y : &a.th)
                                  : (v == 3 ? &b.x : v == 4 ? &b.y : &b.th);
                *p += 1e-6;
                double rp[3];
                residual(a, b, e.meas, rp);
                for (int d = 0; d < 3; ++d) { jac[d][v] = (rp[d] - r[d]) / 1e-6; }
            }
            const int idx[6] = {3 * e.i, 3 * e.i + 1, 3 * e.i + 2, 3 * e.j, 3 * e.j + 1, 3 * e.j + 2};
            for (int u = 0; u < 6; ++u) {
                for (int d = 0; d < 3; ++d) { g[idx[u]] -= jac[d][u] * w[d] * r[d]; }
                for (int v = 0; v < 6; ++v) {
                    double s = 0.0;
                    for (int d = 0; d < 3; ++d) { s += jac[d][u] * w[d] * jac[d][v]; }
                    h[idx[u] * n + idx[v]] += s;
                }
            }
        }
        const std::vector<double> dx = solveSpd(h, g, n);
        for (std::size_t k = 0; k < x.size(); ++k) {
            x[k].x += dx[3 * k];
            x[k].y += dx[3 * k + 1];
            x[k].th = rb::wrapAngle(x[k].th + dx[3 * k + 2]);
        }
    }
    return chi2;
}

void report(const char* name, const std::vector<rb::Pose>& x, const std::vector<rb::Pose>& truth)
{
    double sum = 0.0;
    double worst = 0.0;
    for (std::size_t k = 0; k < x.size(); ++k) {
        const double e = std::hypot(x[k].x - truth[k].x, x[k].y - truth[k].y);
        sum += e * e;
        worst = std::fmax(worst, e);
    }
    std::printf("%-30s RMS %.3f m  worst %.3f m  last node %.3f m\n", name,
                std::sqrt(sum / static_cast<double>(x.size())), worst,
                std::hypot(x.back().x - truth.back().x, x.back().y - truth.back().y));
}

int main()
{
    const std::vector<rb::Pose> path = rb::densify(rb::routeWaypoints(), 0.05);
    const rb::OdoNoise noise;
    rb::Rng rngOdo(530);
    rb::Rng rngLoop(531);
    std::vector<rb::Pose> truth;
    std::vector<rb::Pose> odo;
    rb::Pose odoPose = path[0];
    double sinceScan = 1e9;
    for (std::size_t k = 0; k < path.size(); ++k) {
        if (k > 0) {
            const rb::Delta d = rb::between(path[k - 1], path[k]);
            const rb::Delta measured = rb::noisy(d, noise, rngOdo);
            odoPose = rb::compose(odoPose, measured.fwd, 0.0, measured.dth);
            sinceScan += std::hypot(d.fwd, d.left);
        }
        if (sinceScan < 0.25) { continue; }
        sinceScan = 0.0;
        truth.push_back(path[k]);
        odo.push_back(odoPose);
    }
    const int nodes = static_cast<int>(odo.size());
    std::vector<Edge> edges;
    for (int k = 0; k + 1 < nodes; ++k) {              // odometry edges: sigma 5 cm, 0.02 rad
        edges.push_back({k, k + 1, rb::between(odo[k], odo[k + 1]), 1.0 / 0.0025, 1.0 / 0.0004});
    }
    int closures = 0;
    for (int i = 0; i < nodes; ++i) {                  // loop closures: revisits >= 15 nodes later
        for (int j = i + 15; j < nodes; ++j) {
            if (std::hypot(truth[i].x - truth[j].x, truth[i].y - truth[j].y) > 0.3) { continue; }
            rb::Delta m = rb::between(truth[i], truth[j]);
            m.fwd += rngLoop.gauss(0.02);
            m.left += rngLoop.gauss(0.02);
            m.dth += rngLoop.gauss(0.005);
            edges.push_back({i, j, m, 1.0 / 0.0004, 1.0 / 0.000025});   // sigma 2 cm, 0.005 rad
            ++closures;
            if (closures <= 4) { std::printf("loop closure %d: node %d <-> node %d\n", closures, i, j); }
        }
    }
    std::printf("%d nodes, %d odometry edges, %d loop closures\n", nodes, nodes - 1, closures);
    report("odometry only:", odo, truth);
    std::vector<rb::Pose> x = odo;
    for (int step = 1; step <= 4; ++step) {
        const double chi2 = optimise(x, edges, 1);
        std::printf("Gauss-Newton step %d: weighted squared error before the step %.1f\n", step, chi2);
    }
    report("pose graph, correct closures:", x, truth);

    // One false closure: a place recogniser confuses two spots that look alike.
    std::vector<Edge> bad = edges;
    bad.push_back({20, 60, rb::between(truth[20], truth[20]), 1.0 / 0.0004, 1.0 / 0.000025});
    std::printf("false closure: node 20 (%.2f, %.2f) claimed equal to node 60 (%.2f, %.2f)\n",
                truth[20].x, truth[20].y, truth[60].x, truth[60].y);
    std::vector<rb::Pose> y = odo;
    optimise(y, bad, 5);
    report("pose graph, one false closure:", y, truth);
    return 0;
}
