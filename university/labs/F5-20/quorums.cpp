// Why a configuration change needs care: count pairs of majorities that share
// no server, for a direct switch from C_old to C_new and for joint consensus.
#include <cstdio>
#include <vector>

using Set = unsigned;  // bit i set = server i+1 is in the set

static int count(Set s)
{
    int n = 0;
    for (; s != 0; s &= s - 1) {
        ++n;
    }
    return n;
}

static bool majorityOf(Set group, Set s) { return 2 * count(group & s) > count(group); }

static void show(Set s)
{
    std::printf("{");
    bool first = true;
    for (int i = 0; i < 8; ++i) {
        if (s >> i & 1U) {
            std::printf("%s%d", first ? "" : ",", i + 1);
            first = false;
        }
    }
    std::printf("}");
}

int main()
{
    const Set oldCfg = 0b00111;  // {1,2,3}
    const Set newCfg = 0b11111;  // {1,2,3,4,5}
    const Set all = oldCfg | newCfg;

    // Every set of servers that is a majority under each rule.
    std::vector<Set> qOld, qNew, qJoint;
    for (Set s = 1; s <= all; ++s) {
        if ((s & ~all) != 0) {
            continue;
        }
        if (majorityOf(oldCfg, s)) {
            qOld.push_back(s);
        }
        if (majorityOf(newCfg, s)) {
            qNew.push_back(s);
        }
        if (majorityOf(oldCfg, s) && majorityOf(newCfg, s)) {
            qJoint.push_back(s);
        }
    }
    std::printf("quorums of C_old {1,2,3}: %zu, of C_new {1,2,3,4,5}: %zu, joint: %zu\n",
                qOld.size(), qNew.size(), qJoint.size());

    // Direct switch: one group of servers may still use C_old while another uses C_new.
    int disjoint = 0;
    for (Set a : qOld) {
        for (Set b : qNew) {
            if ((a & b) == 0) {
                if (disjoint < 3) {
                    std::printf("  direct switch: C_old majority ");
                    show(a);
                    std::printf(" and C_new majority ");
                    show(b);
                    std::printf(" share no server\n");
                }
                ++disjoint;
            }
        }
    }
    std::printf("direct switch: %d disjoint pairs (two leaders possible in one term)\n", disjoint);

    // Joint consensus: during the change, decisions need a joint quorum; before it
    // C_old quorums, after it C_new quorums. Check every pair that can coexist.
    int jointDisjoint = 0;
    for (Set j : qJoint) {
        for (Set a : qOld) {
            jointDisjoint += (j & a) == 0;
        }
        for (Set b : qNew) {
            jointDisjoint += (j & b) == 0;
        }
        for (Set k : qJoint) {
            jointDisjoint += (j & k) == 0;
        }
    }
    std::printf("joint consensus: %d disjoint pairs\n", jointDisjoint);
    return 0;
}
