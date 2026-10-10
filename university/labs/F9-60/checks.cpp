// checks.cpp - recomputes the numbers quoted in the text of F9-60.
#include <cmath>
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"

static Mat ctrb(const Mat& A, const Mat& B)
{
    const std::size_t n = A.r;
    Mat Wc(n, n * B.c);
    Mat blk = B;
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t i = 0; i < n; ++i)
            for (std::size_t j = 0; j < B.c; ++j) Wc(i, k * B.c + j) = blk(i, j);
        blk = A * blk;
    }
    return Wc;
}

int main()
{
    // 1. Worked example: the F0-67 cart (m = 2 kg, c = 0.8 N s/m), state (x, v), input F.
    const Mat A(2, 2, {0, 1, 0, -0.4});
    const Mat B(2, 1, {0, 0.5});
    const Mat Wc = ctrb(A, B);
    std::printf("cart: [B AB] = [[%g, %g], [%g, %g]], det = %g, rank %zu\n",
                Wc(0, 0), Wc(0, 1), Wc(1, 0), Wc(1, 1),
                Wc(0, 0) * Wc(1, 1) - Wc(0, 1) * Wc(1, 0), rank(Wc));
    const Mat Cv(1, 2, {0, 1});
    const Mat Cx(1, 2, {1, 0});
    std::printf("cart: rank O with speed sensor = %zu, with position sensor = %zu\n",
                rank(tr(ctrb(tr(A), tr(Cv)))), rank(tr(ctrb(tr(A), tr(Cx)))));

    // 2. Cart-pole with the angle sensor only: friction is what makes v visible.
    for (double b : {0.1, 0.0}) {
        CartPole p;
        p.b = b;
        const Mat Cth(1, 4, {0, 0, 1, 0});
        std::printf("cart-pole, b = %.1f, angle sensor only: rank O = %zu\n",
                    b, rank(tr(ctrb(tr(cartA(p)), tr(Cth)))));
    }

    // 3. Two equal poles (l = 0.5 m): PBH test at the eigenvalues of the difference mode.
    const double M = 1.0, m = 0.2, bb = 0.1, g = 9.81, l = 0.5;
    Mat A2(6, 6);
    A2(0, 1) = 1.0;
    A2(1, 1) = -bb / M;
    A2(1, 2) = -m * g / M;
    A2(1, 4) = -m * g / M;
    A2(2, 3) = 1.0;
    A2(4, 5) = 1.0;
    for (std::size_t r : {3u, 5u}) {
        A2(r, 1) = bb / (M * l);
        A2(r, 2) = m * g / (M * l);
        A2(r, 4) = m * g / (M * l);
        A2(r, r - 1) += g / l;
    }
    const Mat B2(6, 1, {0, 1.0 / M, 0, -1.0 / (M * l), 0, -1.0 / (M * l)});
    std::printf("two equal poles: rank [B AB ... A^5 B] = %zu\n", rank(ctrb(A2, B2)));
    printEig("two equal poles, eig(A)", eig(A2));
    const double lam = std::sqrt(g / l);
    for (double s : {lam, -lam, 5.0}) {
        Mat H(6, 7);
        for (std::size_t i = 0; i < 6; ++i) {
            for (std::size_t j = 0; j < 6; ++j) H(i, j) = (i == j ? s : 0.0) - A2(i, j);
            H(i, 6) = B2(i, 0);
        }
        std::printf("PBH: rank [sI - A, B] at s = %+.4f is %zu\n", s, rank(H));
    }
    return 0;
}
