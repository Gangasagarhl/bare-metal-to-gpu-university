// good.cpp - F11-11: the same job written to a coding standard. Built and run by
// run_lab.sh under -fsanitize=address,undefined (clean), and checked by
// clang-tidy in run.sh (no warnings). Every problem of bad.cc is gone:
//   - parsing reports errors (std::from_chars) instead of atoi;
//   - comparison uses '==';
//   - every variable is initialised;
//   - ownership is a std::vector, so there is no manual free at all.
#include <charconv>
#include <cstdio>
#include <optional>
#include <string_view>
#include <vector>

namespace {

std::optional<int> parse_count(std::string_view text)
{
    int value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) return std::nullopt;  // report errors
    if (value < 0 || value > (1 << 20)) return std::nullopt;                         // bound it
    return value;
}

}  // namespace

int main(int argc, char** argv)
{
    std::string_view text = (argc > 1) ? argv[1] : "16";
    std::optional<int> n = parse_count(text);
    if (!n) { std::printf("bad count\n"); return 1; }

    std::vector<char> buffer(static_cast<std::size_t>(*n), 0);   // owns its memory; no free needed
    std::printf("count=%d buffer=%zu bytes\n", *n, buffer.size());
    return 0;
}
