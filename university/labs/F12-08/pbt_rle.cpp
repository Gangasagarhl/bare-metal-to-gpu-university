// pbt_rle.cpp - property: decoding an encoding gives back the input ("round trip"),
// checked on random byte strings that contain long runs, for both versions of the encoder.
#include "pbt.hpp"
#include "rle.hpp"

#include <string>

namespace {

pbt::Gen<rle::Bytes> runs_of_bytes()
{
    pbt::Gen<rle::Bytes> g;
    g.make = [](pbt::Rng& rng, int size) {
        const std::uint8_t alphabet[] = {0, 1, 7, 255};  // few values, so runs are common
        rle::Bytes v;
        const int runs = 1 + static_cast<int>(pbt::below(rng, 4));
        for (int r = 0; r < runs; ++r) {
            const auto value = alphabet[pbt::below(rng, 4)];
            const auto len = 1 + pbt::below(rng, 4 * static_cast<std::uint64_t>(size));
            v.insert(v.end(), len, value);
        }
        return v;
    };
    g.shrink = [](const rle::Bytes& v) {
        auto out = pbt::remove_chunks(v);
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (v[i] != 0) {  // try a smaller value: 0
                rle::Bytes c = v;
                c[i] = 0;
                out.push_back(c);
            }
        }
        return out;
    };
    g.show = [](const rle::Bytes& v) {  // print runs as value*count
        std::string s = "[";
        for (std::size_t i = 0; i < v.size();) {
            std::size_t j = i;
            while (j < v.size() && v[j] == v[i]) {
                ++j;
            }
            s += (i == 0 ? "" : " ") + std::to_string(v[i]) + "*" + std::to_string(j - i);
            i = j;
        }
        return s + "] (" + std::to_string(v.size()) + " bytes)";
    };
    return g;
}

}  // namespace

int main()
{
    const pbt::Config cfg{2026, 200, 100};
    const auto gen = runs_of_bytes();
    auto round_trip_v1 = [](const rle::Bytes& x) { return rle::decode(rle::encode_v1(x)) == x; };
    auto round_trip = [](const rle::Bytes& x) { return rle::decode(rle::encode(x)) == x; };
    const bool v1 = pbt::for_all("round_trip(encode_v1)", cfg, gen, round_trip_v1);
    const bool v2 = pbt::for_all("round_trip(encode)", cfg, gen, round_trip);
    return (!v1 && v2) ? 0 : 1;  // the lab expects: v1 fails, the fixed version passes
}
