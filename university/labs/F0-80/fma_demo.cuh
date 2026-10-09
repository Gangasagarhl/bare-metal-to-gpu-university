// F0-80: a one-line kernel used only to read the PTX nvcc generates for a * x + b (run.sh step
// fmad_ptx).
__global__ void axpb(const float* x, float* y, float a, float b, int n)
{
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n) {
        y[i] = a * x[i] + b;
    }
}
