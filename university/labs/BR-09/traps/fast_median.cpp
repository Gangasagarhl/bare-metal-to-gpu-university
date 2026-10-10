// Faster median: nth_element instead of a full sort.
// (Pasted from a reply on a question-and-answer website in 2026; author unknown.)
#include <algorithm>
#include <vector>

double fast_median(std::vector<double> xs)
{
    const auto mid = xs.begin() + static_cast<std::ptrdiff_t>(xs.size() / 2);
    std::nth_element(xs.begin(), mid, xs.end());
    return *mid;
}
