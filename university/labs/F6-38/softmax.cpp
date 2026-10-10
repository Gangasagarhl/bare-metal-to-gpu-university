// F6-38 Listing 1: three ways to compute softmax of one row in float, compared with a
// double-precision reference. naive: exp(x) / sum(exp(x)), no max subtracted (overflows).
// safe: three passes (max, sum, write). online: two passes, max and sum in one pass
// (the online normaliser), then the write pass.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

struct Result
{
    std::vector<float> y;
    int readsPerElement;   // how many times each input element is read from memory
};

Result naiveSoftmax(const std::vector<float>& x)
{
    float sum = 0.0f;
    for (float v : x) {
        sum += std::exp(v);
    }
    std::vector<float> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] = std::exp(x[i]) / sum;
    }
    return {y, 2};
}

Result safeSoftmax(const std::vector<float>& x)
{
    float m = -std::numeric_limits<float>::infinity();
    for (float v : x) {
        m = std::fmax(m, v);
    }
    float sum = 0.0f;
    for (float v : x) {
        sum += std::exp(v - m);
    }
    std::vector<float> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] = std::exp(x[i] - m) / sum;
    }
    return {y, 3};
}

Result onlineSoftmax(const std::vector<float>& x)
{
    float m = -std::numeric_limits<float>::infinity();   // running maximum
    float d = 0.0f;                                       // running normaliser
    for (float v : x) {
        const float mNew = std::fmax(m, v);
        d = d * std::exp(m - mNew) + std::exp(v - mNew);  // rescale the old sum, add the new term
        m = mNew;
    }
    std::vector<float> y(x.size());
    for (std::size_t i = 0; i < x.size(); ++i) {
        y[i] = std::exp(x[i] - m) / d;
    }
    return {y, 2};
}

double maxAbsError(const std::vector<float>& y, const std::vector<float>& x)
{
    double m = -1e300;
    for (float v : x) {
        m = std::fmax(m, static_cast<double>(v));
    }
    double s = 0.0;
    for (float v : x) {
        s += std::exp(static_cast<double>(v) - m);
    }
    double err = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double ref = std::exp(static_cast<double>(x[i]) - m) / s;
        const double e = std::fabs(static_cast<double>(y[i]) - ref);
        if (std::isnan(e)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        err = std::fmax(err, e);
    }
    return err;
}

int main()
{
    // each case: name, number of elements n, start value, step (x[i] = start + step * (i % 7))
    std::string name;
    std::size_t n = 0;
    float start = 0.0f;
    float step = 0.0f;
    std::printf("%-12s %6s %8s | %-24s | %-24s | %-24s\n", "case", "n", "max x",
                "naive (2 reads/elem)", "safe (3 reads/elem)", "online (2 reads/elem)");
    while (std::cin >> name >> n >> start >> step) {
        std::vector<float> x(n);
        float mx = -std::numeric_limits<float>::infinity();
        for (std::size_t i = 0; i < n; ++i) {
            x[i] = start + step * static_cast<float>(i % 7);
            mx = std::fmax(mx, x[i]);
        }
        const Result r[3] = {naiveSoftmax(x), safeSoftmax(x), onlineSoftmax(x)};
        std::printf("%-12s %6zu %8.1f", name.c_str(), n, static_cast<double>(mx));
        for (const Result& res : r) {
            std::printf(" | max error %-14.3g", maxAbsError(res.y, x));
        }
        std::printf("\n");
    }
    std::printf("float overflow threshold for exp: exp(88) = %g, exp(89) = %g\n",
                static_cast<double>(std::exp(88.0f)), static_cast<double>(std::exp(89.0f)));
    return 0;
}
