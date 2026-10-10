// Four tiny hand-written histories of one register x (initially 0).
#include <cstdio>

#include "linearizability.h"

static raft::HistoryOp op(int process, bool isPut, int value, raft::Time call, raft::Time reply)
{
    raft::HistoryOp h;
    h.process = process;
    h.isPut = isPut;
    h.value = value;
    h.invoke = call;
    h.response = reply;  // -1: no reply
    return h;
}

static void judge(const char* title, const std::vector<raft::HistoryOp>& history)
{
    std::printf("%s\n", title);
    lin::printHistory(history);
    lin::Checker checker(history);
    std::vector<int> order;
    if (checker.check(order)) {
        std::printf("  -> linearizable, e.g. in this order:");
        for (int i : order) {
            const raft::HistoryOp& h = checker.ops()[i];
            std::printf(" %s%d", h.isPut ? "put " : "get->", h.value);
        }
        std::printf("\n\n");
    } else {
        std::printf("  -> NOT linearizable\n\n");
    }
}

int main()
{
    judge("H1: the get overlaps the put",
          {op(1, true, 1, 0, 10), op(2, false, 0, 5, 15)});
    judge("H2: the get starts after the put has finished, but returns the old value",
          {op(1, true, 1, 0, 10), op(2, false, 0, 20, 30)});
    judge("H3: a put never answered; a reader sees 0 after another reader saw 1",
          {op(1, true, 1, 0, -1), op(2, false, 1, 20, 30), op(3, false, 0, 40, 50)});
    judge("H4: a put never answered; 0 is seen first, then 1",
          {op(1, true, 1, 0, -1), op(2, false, 0, 20, 30), op(3, false, 1, 40, 50)});
    return 0;
}
