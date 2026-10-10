// F8-15 Listing 1: memory for model states per GPU and communication per step, for plain data
// parallelism and three levels of sharding (stage 1: optimizer states; stage 2: also gradients;
// stage 3: also parameters). Activations and temporary buffers are NOT counted.
// Input lines: label params ranks bytesPerParam bytesPerGrad bytesOptimizerPerParam budgetGiB
// Communication is counted as bytes each GPU sends per step with ring algorithms (F8-02):
// all-reduce 2 (N-1)/N x S, reduce-scatter and all-gather (N-1)/N x S each, for S bytes.
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string label;
    double psi, n, bp, bg, bo, budgetGiB;
    const double gib = 1024.0 * 1024.0 * 1024.0;
    while (std::cin >> label >> psi >> n >> bp >> bg >> bo >> budgetGiB) {
        const double perStage[4] = {
            (bp + bg + bo) * psi,                    // plain: everything replicated
            (bp + bg) * psi + bo * psi / n,          // stage 1
            bp * psi + (bg + bo) * psi / n,          // stage 2
            (bp + bg + bo) * psi / n,                // stage 3
        };
        const double perParam[4] = {bp + bg + bo, bp + bg + bo / n, bp + (bg + bo) / n, (bp + bg + bo) / n};
        const double ring = (n - 1) / n;
        const double commBytes[4] = {
            2 * ring * bg * psi,                     // all-reduce of the gradients
            2 * ring * bg * psi,                     // reduce-scatter gradients + all-gather parameters
            2 * ring * bg * psi,
            3 * ring * bg * psi,                     // + one more all-gather of parameters (backward)
        };
        std::printf("%s: %.3g parameters, N = %.0f GPUs, bytes per parameter: %.0f (parameter) + %.0f (gradient)"
                    " + %.0f (optimizer); budget %.0f GiB per GPU (model states only)\n",
                    label.c_str(), psi, n, bp, bg, bo, budgetGiB);
        std::printf("  stage                       model states per GPU   sent per GPU per step   largest model that fits the budget\n");
        const char* names[4] = {"0 plain data parallel   ", "1 shard optimizer states", "2 + shard gradients     ",
                                "3 + shard parameters    "};
        for (int s = 0; s < 4; ++s) {
            std::printf("  %s %14.2f GiB  %17.2f GiB  %22.3g parameters\n", names[s], perStage[s] / gib,
                        commBytes[s] / gib, budgetGiB * gib / perParam[s]);
        }
    }
    return 0;
}
