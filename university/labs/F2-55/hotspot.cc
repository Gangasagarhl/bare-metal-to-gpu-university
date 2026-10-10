// hotspot.cc - a small word-count program to profile: where does its time go?
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

[[gnu::noinline]] std::string generateText(std::size_t words)
{
    static char const* const vocabulary[] = {"cache", "line", "branch", "vector", "lane",
                                             "sample", "profile", "roof", "kernel", "loop"};
    std::mt19937 rng(3);
    std::uniform_int_distribution<int> pick(0, 9);
    std::string text;
    for (std::size_t i = 0; i < words; ++i) {
        text += vocabulary[pick(rng)];
        text += std::to_string(pick(rng));  // "cache7", "loop2": 100 distinct words
        text += ' ';
    }
    return text;
}

[[gnu::noinline]] std::vector<std::string> splitWords(std::string const& text)
{
    std::vector<std::string> words;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == ' ') {
            words.push_back(text.substr(start, i - start));
            start = i + 1;
        }
    }
    return words;
}

[[gnu::noinline]] std::map<std::string, long> countWords(std::vector<std::string> const& words)
{
    std::map<std::string, long> counts;
    for (auto const& w : words) {
        ++counts[w];
    }
    return counts;
}

[[gnu::noinline]] std::vector<std::pair<long, std::string>> topWords(std::map<std::string, long> const& counts)
{
    std::vector<std::pair<long, std::string>> v;
    for (auto const& [word, n] : counts) {
        v.emplace_back(n, word);
    }
    std::sort(v.begin(), v.end(), [](auto const& a, auto const& b) { return a.first > b.first; });
    return v;
}

int main()
{
    std::string const text = generateText(2'000'000);
    auto const words = splitWords(text);
    auto const counts = countWords(words);
    auto const top = topWords(counts);
    std::printf("%zu words, %zu distinct; most frequent: %s (%ld)\n", words.size(), counts.size(),
                top.front().second.c_str(), top.front().first);
    return 0;
}
