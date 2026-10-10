// bottleneck.cpp - BR-10 Listing 9: a model of one shared file server feeding k nodes.
// All numbers are exercise values chosen for the arithmetic, not properties of any product.
#include <cstdio>

int main()
{
    double const inputGB = 600;        // the job reads this much input in total
    double const sharedGBs = 2;        // the shared file server's total bandwidth
    double const localGBs = 1;         // one node's local disk, if the input is staged there
    double const computeS = 3600;      // compute time of the whole job on one node
    std::printf("input %.0f GB; shared server %.0f GB/s in total; local disk %.0f GB/s per node;"
                " compute %.0f node-seconds\n", inputGB, sharedGBs, localGBs, computeS);
    std::printf("    k   shared: read + compute = time   speedup  |   local: read + compute ="
                " time   speedup\n");
    double const shared1 = inputGB / sharedGBs + computeS;
    double const local1 = inputGB / localGBs + computeS;
    for (int k = 1; k <= 64; k *= 2) {
        double const sRead = inputGB / sharedGBs;      // everybody shares one server
        double const lRead = inputGB / (k * localGBs);  // every node reads its own share
        double const comp = computeS / k;
        std::printf("   %2d   %6.1f + %6.1f = %6.1f s   %6.2f   |   %6.1f + %6.1f = %6.1f s   %6.2f\n",
                    k, sRead, comp, sRead + comp, shared1 / (sRead + comp), lRead, comp,
                    lRead + comp, local1 / (lRead + comp));
    }
    std::printf("limit of the shared column as k grows: %.1f s of reading, speedup at most %.1f\n",
                inputGB / sharedGBs, shared1 / (inputGB / sharedGBs));
    return 0;
}
