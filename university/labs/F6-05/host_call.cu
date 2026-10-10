// F6-05 Listing 5: does not compile, on purpose. A kernel may only call functions
// that are compiled for the device.
int twice(int x)            // an ordinary (host) function
{
    return 2 * x;
}

__global__ void useTwice(int* v)
{
    v[threadIdx.x] = twice(v[threadIdx.x]);
}

int main()
{
    return 0;
}
