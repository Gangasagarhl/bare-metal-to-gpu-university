// pbt_ring.cpp - model-based property: any sequence of push/pop operations gives the same
// results on the ring buffer as on a simple, obviously correct model (std::deque).
#include "pbt.hpp"
#include "ringbuf.hpp"

#include <deque>
#include <string>
#include <vector>

namespace {

struct Op {
    bool push;
    int value;
};
using Ops = std::vector<Op>;

bool same_as_model(const Ops& ops)
{
    RingBuffer ring;
    std::deque<int> model;
    for (const Op& op : ops) {
        if (op.push) {
            const bool model_ok = model.size() < RingBuffer::kCapacity;
            if (model_ok) {
                model.push_back(op.value);
            }
            if (ring.push(op.value) != model_ok) {
                return false;
            }
        } else {
            std::optional<int> want;
            if (!model.empty()) {
                want = model.front();
                model.pop_front();
            }
            if (ring.pop() != want) {
                return false;
            }
        }
    }
    return true;
}

pbt::Gen<Ops> op_sequences()
{
    pbt::Gen<Ops> g;
    g.make = [](pbt::Rng& rng, int size) {
        Ops ops(static_cast<std::size_t>(size));
        for (Op& op : ops) {
            op.push = pbt::below(rng, 10) < 6;
            op.value = static_cast<int>(pbt::below(rng, 100));
        }
        return ops;
    };
    g.shrink = [](const Ops& ops) {
        auto out = pbt::remove_chunks(ops);
        for (std::size_t i = 0; i < ops.size(); ++i) {
            if (ops[i].push && ops[i].value != 0) {
                Ops c = ops;
                c[i].value = 0;
                out.push_back(c);
            }
        }
        return out;
    };
    g.show = [](const Ops& ops) {
        std::string s;
        for (const Op& op : ops) {
            s += op.push ? "push(" + std::to_string(op.value) + ") " : "pop ";
        }
        return s + "(" + std::to_string(ops.size()) + " operations)";
    };
    return g;
}

}  // namespace

int main()
{
    const pbt::Config cfg{2026, 300, 40};
    const bool ok = pbt::for_all("ring_matches_model", cfg, op_sequences(), same_as_model);
    return ok ? 1 : 0;  // the lab expects the planted bug to be found
}
