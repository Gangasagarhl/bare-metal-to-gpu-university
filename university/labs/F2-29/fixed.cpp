// fixed.cpp: the four bugs of this chapter's lab, fixed. Built by run_lab.sh with
// -fsanitize=address,undefined, so a clean run is evidence that the fixes work.
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

int sum_readings(const std::vector<int>& readings)
{
    int sum = 0;
    for (int r : readings) {                  // no index, so no off-by-one
        sum += r;
    }
    return sum;
}

std::int64_t factorial(int n)                 // 13! = 6227020800 needs 64 bits
{
    std::int64_t result = 1;
    for (int k = 2; k <= n; ++k) {
        result *= k;
    }
    return result;
}

std::uint32_t mask(int bits)                  // defined for 0..32
{
    return bits >= 32 ? UINT32_MAX : (std::uint32_t{1} << bits) - 1u;
}

struct Person
{
    std::string name;
    std::weak_ptr<Person> friend_of;          // non-owning: no ownership cycle
};

int main()
{
    std::printf("sum = %d\n", sum_readings({10, 20, 30, 40, 50}));
    std::printf("13! = %lld\n", static_cast<long long>(factorial(13)));
    std::printf("mask(32) = %u\n", mask(32));
    auto ada = std::make_shared<Person>();
    auto bo = std::make_shared<Person>();
    ada->name = "Ada";
    bo->name = "Bo";
    ada->friend_of = bo;
    bo->friend_of = ada;
    std::printf("%s's friend is %s\n", ada->name.c_str(), ada->friend_of.lock()->name.c_str());
    return 0;
}
