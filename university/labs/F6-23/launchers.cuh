// F6-23: launchers for the register-tiled variants, shared by the GPU harness and the
// CPU emulator check. LAUNCH is a macro so the same line works with <<< >>> and launch().
#pragma once

template <int BM, int BN, int BK, int TM, int TN, bool VEC, int MINB = 1>
void launchRegTile(const float* A, const float* B, float* C, int M, int N, int K)
{
    const dim3 block((BM / TM) * (BN / TN));
    const dim3 grid((N + BN - 1) / BN, (M + BM - 1) / BM);
    if (VEC && (K % 4 != 0 || N % 4 != 0)) {       // float4 loads need 16-byte aligned rows
        LAUNCH((sgemmRegTile<BM, BN, BK, TM, TN, false, MINB>), grid, block, A, B, C, M, N, K);
    } else {
        LAUNCH((sgemmRegTile<BM, BN, BK, TM, TN, VEC, MINB>), grid, block, A, B, C, M, N, K);
    }
}
