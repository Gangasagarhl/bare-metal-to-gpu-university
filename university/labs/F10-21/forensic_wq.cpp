// forensic_wq.cpp - evidence for the F10-21 forensic lab: the work-queue model after a
// change made in a feature branch.
#include "wq_model.h"

int main()
{
    using namespace wq_model;
    Config b = configA();
    b.title = "B: feature branch";
    b.items[5].wq = 1;   // the change under investigation
    simulate(b, 200000, 1);
    return 0;
}
