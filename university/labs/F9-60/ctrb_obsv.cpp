// ctrb_obsv.cpp - controllability and observability of the cart-pole for three sensor sets.
#include <cmath>
#include <cstdio>
#include "cartpole.hpp"
#include "lin.hpp"

// [B, AB, A^2 B, ..., A^(n-1) B]
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

// [C; CA; CA^2; ...; CA^(n-1)]
static Mat obsv(const Mat& A, const Mat& C)
{
    return tr(ctrb(tr(A), tr(C)));  // duality: (A, C) observable <=> (A^T, C^T) controllable
}

static void report(const char* what, const Mat& W)
{
    const Mat G = (W.r <= W.c) ? W * tr(W) : tr(W) * W;  // the smaller Gram matrix
    const auto sv2 = symEig(G);  // eigenvalues of G = squared singular values of W
    std::printf("%-26s rank %zu of %zu; singular values:", what, rank(W), G.r);
    for (double v : sv2) std::printf(" %.3g", std::sqrt(std::fmax(v, 0.0)));
    std::printf("\n");
}

int main()
{
    const CartPole p;
    const Mat A = cartA(p);
    const Mat B = cartB(p);
    const Mat Wc = ctrb(A, B);
    printMat("controllability matrix [B AB A^2B A^3B]", Wc);
    report("force on the cart:", Wc);

    const Mat Cx(1, 4, {1, 0, 0, 0});
    const Mat Cth(1, 4, {0, 0, 1, 0});
    const Mat Cboth(2, 4, {1, 0, 0, 0,
                           0, 0, 1, 0});
    report("sensor: cart position x", obsv(A, Cx));
    report("sensor: pole angle th", obsv(A, Cth));
    report("sensors: x and th", obsv(A, Cboth));
    printMat("observability matrix for th only [C; CA; CA^2; CA^3]", obsv(A, Cth));
    return 0;
}
