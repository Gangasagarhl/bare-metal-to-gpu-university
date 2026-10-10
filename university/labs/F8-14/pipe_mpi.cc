// F8-14 Listing 2: a real GPipe pipeline with MPI. Rank s is stage s and owns one layer,
// y = tanh(W_s x + b_s). The batch is cut into M micro-batches; every stage runs all forwards,
// then all backwards, passing activations forward and gradients back with MPI_Send/MPI_Recv.
// A sleep of tf (forward) or 2*tf (backward) milliseconds stands in for the GPU's work, so the
// timing is that of a pipeline whose stages are equally slow. Each stage checks its weight
// gradient against the whole model computed alone, and rank 0 reports the measured bubble.
// Arguments: M=<micro-batches> tf=<ms> slow=<stage>:<factor> (forensic lab: one slow stage).
#include <mpi.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

constexpr int kW = 16;        // layer width
constexpr int kMb = 2;        // samples per micro-batch
using Vec = std::vector<float>;
using Clock = std::chrono::steady_clock;

std::uint64_t mix(std::uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

Vec randomVec(int n, std::uint64_t seed, float scale)
{
    Vec v(n);
    for (int i = 0; i < n; ++i) v[i] = scale * (static_cast<float>(mix((seed << 32) + i) >> 40) / 8388608.0f - 1.0f);
    return v;
}

// layer s: weights W (kW x kW) then bias (kW), as one vector
Vec layer(int s) { return randomVec(kW * kW + kW, 100 + s, 0.4f); }

Vec forward(const Vec& p, const Vec& x)            // x: kMb rows of kW
{
    Vec y(kMb * kW);
    for (int r = 0; r < kMb; ++r) {
        for (int i = 0; i < kW; ++i) {
            float z = p[kW * kW + i];
            for (int j = 0; j < kW; ++j) z += p[i * kW + j] * x[r * kW + j];
            y[r * kW + i] = std::tanh(z);
        }
    }
    return y;
}

// given x, y = forward(x) and dL/dy, adds dL/dp to g and returns dL/dx
Vec backward(const Vec& p, const Vec& x, const Vec& y, const Vec& dy, Vec& g)
{
    Vec dx(kMb * kW, 0.0f);
    for (int r = 0; r < kMb; ++r) {
        for (int i = 0; i < kW; ++i) {
            const float dz = dy[r * kW + i] * (1.0f - y[r * kW + i] * y[r * kW + i]);
            g[kW * kW + i] += dz;
            for (int j = 0; j < kW; ++j) {
                g[i * kW + j] += dz * x[r * kW + j];
                dx[r * kW + j] += dz * p[i * kW + j];
            }
        }
    }
    return dx;
}

Vec input(int m) { return randomVec(kMb * kW, 1000 + m, 1.0f); }
Vec target(int m) { return randomVec(kMb * kW, 2000 + m, 0.5f); }

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int s = 0, S = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &s);
    MPI_Comm_size(MPI_COMM_WORLD, &S);
    int M = 4, tf = 20, slowStage = -1, slowFactor = 1;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("M=", 0) == 0) M = std::atoi(a.c_str() + 2);
        if (a.rfind("tf=", 0) == 0) tf = std::atoi(a.c_str() + 3);
        if (a.rfind("slow=", 0) == 0) std::sscanf(a.c_str() + 5, "%d:%d", &slowStage, &slowFactor);
    }
    const int myTf = s == slowStage ? tf * slowFactor : tf;
    const Vec p = layer(s);
    Vec g(p.size(), 0.0f);
    std::vector<Vec> xs(M), ys(M);
    double busyMs = 0;
    auto work = [&](int ms) {                       // the stand-in for GPU work, timed
        const auto t0 = Clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        busyMs += std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    };

    MPI_Barrier(MPI_COMM_WORLD);
    const auto start = Clock::now();
    for (int m = 0; m < M; ++m) {                   // all forwards
        xs[m] = Vec(kMb * kW);
        if (s == 0) xs[m] = input(m);
        else MPI_Recv(xs[m].data(), kMb * kW, MPI_FLOAT, s - 1, m, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        ys[m] = forward(p, xs[m]);
        work(myTf);
        if (s < S - 1) MPI_Send(ys[m].data(), kMb * kW, MPI_FLOAT, s + 1, m, MPI_COMM_WORLD);
    }
    for (int m = 0; m < M; ++m) {                   // all backwards
        Vec dy(kMb * kW);
        if (s == S - 1) {
            const Vec t = target(m);                // loss = 0.5 * sum (y - t)^2
            for (int i = 0; i < kMb * kW; ++i) dy[i] = ys[m][i] - t[i];
        } else {
            MPI_Recv(dy.data(), kMb * kW, MPI_FLOAT, s + 1, 1000 + m, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        const Vec dx = backward(p, xs[m], ys[m], dy, g);
        work(2 * myTf);
        if (s > 0) MPI_Send(dx.data(), kMb * kW, MPI_FLOAT, s - 1, 1000 + m, MPI_COMM_WORLD);
    }
    const double spanMs = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

    // reference: the whole model on this process, all micro-batches, gradient of layer s only
    std::vector<Vec> ps(S);
    for (int k = 0; k < S; ++k) ps[k] = layer(k);
    Vec gref(p.size(), 0.0f);
    for (int m = 0; m < M; ++m) {
        std::vector<Vec> act(S + 1);
        act[0] = input(m);
        for (int k = 0; k < S; ++k) act[k + 1] = forward(ps[k], act[k]);
        Vec d(kMb * kW);
        const Vec t = target(m);
        for (int i = 0; i < kMb * kW; ++i) d[i] = act[S][i] - t[i];
        for (int k = S - 1; k >= s; --k) {
            Vec scratch(p.size(), 0.0f);
            d = backward(ps[k], act[k], act[k + 1], d, k == s ? gref : scratch);
        }
    }
    float diff = 0.0f;
    for (std::size_t i = 0; i < g.size(); ++i) diff = std::fmax(diff, std::fabs(g[i] - gref[i]));

    std::vector<double> busy(S), span(S);
    std::vector<float> diffs(S);
    MPI_Gather(&busyMs, 1, MPI_DOUBLE, busy.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&spanMs, 1, MPI_DOUBLE, span.data(), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Gather(&diff, 1, MPI_FLOAT, diffs.data(), 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    if (s == 0) {
        double total = 0, sumBusy = 0;
        for (int k = 0; k < S; ++k) { total = std::fmax(total, span[k]); sumBusy += busy[k]; }
        std::printf("S = %d stages, M = %d micro-batches, tf = %d ms, tb = %d ms%s\n", S, M, tf, 2 * tf,
                    slowStage >= 0 ? (" (stage " + std::to_string(slowStage) + " x" + std::to_string(slowFactor) + ")").c_str() : "");
        std::printf("stage   busy ms   idle ms   max |grad - grad_ref|\n");
        for (int k = 0; k < S; ++k) {
            std::printf("%5d  %8.0f  %8.0f   %.3g\n", k, busy[k], total - busy[k], diffs[k]);
        }
        const double measured = 1.0 - sumBusy / (S * total);
        const double formula = static_cast<double>(S - 1) / (M + S - 1);
        std::printf("pipeline time %.0f ms; measured bubble %.3f; GPipe formula (S-1)/(M+S-1) = %.3f; ", total, measured, formula);
        std::printf("%s\n", std::fabs(measured - formula) <= 0.2 * formula ? "within 20 %" : "NOT within 20 %");
        float worst = 0.0f;
        for (float d : diffs) worst = std::fmax(worst, d);
        std::printf("gradients %s the single-process model (largest difference %.3g, tolerance 1e-5)\n",
                    worst <= 1e-5f ? "MATCH" : "DO NOT MATCH", worst);
    }
    MPI_Finalize();
    return 0;
}
