// F7-09 Listing 2 (raw/branchy.hip): a kernel with a branch, written WITHOUT the HIP headers
// (compiler builtins only), so the same source can be compiled for RDNA in wave32 and wave64.
// Kernel only: run.sh compiles it to assembly; it is never linked or run.
__attribute__((global)) void branchy(float* y, int n)
{
    int i = __builtin_amdgcn_workgroup_id_x() * 256 + __builtin_amdgcn_workitem_id_x();
    if (i < n) {
        if (y[i] > 0.0f) {
            y[i] = y[i] * 2.0f;
        } else {
            y[i] = -y[i];
        }
    }
}
