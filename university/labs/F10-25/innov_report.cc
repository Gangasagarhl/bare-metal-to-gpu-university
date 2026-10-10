// innov_report.cc - the analysis a log reviewer does for "estimator rejected GNSS":
// one line per second with heading, GNSS innovations, the largest test ratio and how
// many GNSS samples were fused, then a correlation between innovation and heading.
//   innov_report <file.ulg>
#include <cmath>
#include <cstdio>
#include <exception>

#include "ulog_lite.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: innov_report <file.ulg>\n");
        return 2;
    }
    try {
        const auto log = ulog_lite::read(argv[1]);
        const char* in = "est_innov_model";
        const char* st = "vehicle_state_model";
        std::printf("%s\n  t_s  yaw_deg  innov_n_m  innov_e_m  max_ratio  fused/samples\n", argv[1]);
        const std::size_t n = log.rows(in);
        std::size_t s = 0;
        for (int sec = 1; sec * 5 <= static_cast<int>(n) && sec <= 34; sec += 1) {
            double maxRatio = 0.0;
            double sumN = 0.0;
            double sumE = 0.0;
            int fused = 0;
            int count = 0;
            for (std::size_t i = static_cast<std::size_t>((sec - 1) * 5); i < static_cast<std::size_t>(sec * 5); ++i) {
                maxRatio = std::fmax(maxRatio, log.get(in, i, "test_ratio"));
                sumN += log.get(in, i, "innov_n");
                sumE += log.get(in, i, "innov_e");
                fused += static_cast<int>(log.get(in, i, "fused"));
                ++count;
            }
            while (s + 1 < log.rows(st) && log.get(st, s, "timestamp") < sec * 1e6) {
                ++s;
            }
            std::printf("%5d  %7.1f  %9.2f  %9.2f  %9.2f  %d/%d\n", sec, log.get(st, s, "yaw_deg"), sumN / count,
                        sumE / count, maxRatio, fused, count);
        }
        // Does the north innovation follow cos(yaw)? A lever-arm error dX gives
        // innov_n = dX * cos(yaw) + constant while the estimate has not absorbed it.
        double sxy = 0.0, sxx = 0.0, sx = 0.0, sy = 0.0;
        int m = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const double t = log.get(in, i, "timestamp");
            while (s > 0 && log.get(st, s, "timestamp") > t) {
                --s;
            }
            while (s + 1 < log.rows(st) && log.get(st, s + 1, "timestamp") <= t) {
                ++s;
            }
            const double c = std::cos(log.get(st, s, "yaw_deg") * 3.14159265358979323846 / 180.0);
            const double y = log.get(in, i, "innov_n");
            sx += c;
            sy += y;
            sxx += c * c;
            sxy += c * y;
            ++m;
        }
        const double slope = (m * sxy - sx * sy) / (m * sxx - sx * sx);
        std::printf("least-squares fit innov_n = a * cos(yaw) + b over %d samples: a = %.2f m\n", m, slope);
        return 0;
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
        return 1;
    }
}
