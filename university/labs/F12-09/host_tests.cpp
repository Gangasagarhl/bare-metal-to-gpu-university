// host_tests.cpp - CI stage 1: host unit tests of bitmap.hpp, built with ASan and UBSan.
// One test follows curriculum milestone B3: random allocate/free sequences against a
// reference model (std::set); no double allocation; all frames returned at the end.
#include "bitmap.hpp"

#include <cstdio>
#include <random>
#include <set>
#include <vector>

namespace {

int g_failed = 0;

void check(bool ok, const char* what, int line)
{
    if (!ok) {
        std::printf("  CHECK failed: %s (host_tests.cpp:%d)\n", what, line);
        ++g_failed;
    }
}
#define CHECK(x) check((x), #x, __LINE__)

void exhaust_and_return()
{
    FrameBitmap b(128);
    std::vector<int32_t> got;
    for (int i = 0; i < 128; ++i) {
        got.push_back(b.alloc());
    }
    CHECK(b.alloc() == -1);
    CHECK(b.free_count() == 0);
    for (int32_t f : got) {
        CHECK(b.release(f));
    }
    CHECK(b.free_count() == 128);
}

void refuses_bad_frees()
{
    FrameBitmap b(8);
    const int32_t f = b.alloc();
    CHECK(b.release(f));
    CHECK(!b.release(f));  // double free
    CHECK(!b.release(-1));
    CHECK(!b.release(8));
    CHECK(b.free_count() == 8);
}

void random_against_model()
{
    std::mt19937 rng(302);
    for (int round = 0; round < 50; ++round) {
        FrameBitmap b(200);
        std::set<int32_t> model;  // frames the model says are in use
        for (int step = 0; step < 1000; ++step) {
            if (rng() % 2 == 0) {
                const int32_t f = b.alloc();
                if (model.size() == 200) {
                    CHECK(f == -1);
                } else {
                    CHECK(f >= 0 && model.count(f) == 0);  // never handed out twice
                    model.insert(f);
                }
            } else if (!model.empty()) {
                auto it = model.begin();
                std::advance(it, static_cast<long>(rng() % model.size()));
                CHECK(b.release(*it));
                model.erase(it);
            }
            CHECK(b.free_count() == 200 - model.size());
        }
        for (int32_t f : model) {
            CHECK(b.release(f));
        }
        CHECK(b.free_count() == 200);  // all frames returned
    }
}

}  // namespace

int main()
{
    struct {
        const char* name;
        void (*fn)();
    } tests[] = {{"exhaust_and_return", exhaust_and_return},
                 {"refuses_bad_frees", refuses_bad_frees},
                 {"random_against_model", random_against_model}};
    for (auto& t : tests) {
        const int before = g_failed;
        t.fn();
        std::printf("host test %-22s %s\n", t.name, g_failed == before ? "ok" : "FAILED");
    }
    std::printf("host tests: %d failed checks\n", g_failed);
    return g_failed == 0 ? 0 : 1;
}
