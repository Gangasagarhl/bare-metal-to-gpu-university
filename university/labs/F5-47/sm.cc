// sm.cc - DS403 cluster kernel: apply commands to the key-value table and the job table.
#include "sm.h"
#include "k.h"

namespace {
constexpr int NKEYS = 8;
char keys[NKEYS][8];
char vals[NKEYS][8];
Job jobs[MAX_JOBS + 1];
const Job none{};

int slot(const char* key, bool create)
{
    for (int i = 0; i < NKEYS; ++i)
        if (keys[i][0] != '\0' && kstrcmp(keys[i], key) == 0) return i;
    if (!create) return -1;
    for (int i = 0; i < NKEYS; ++i)
        if (keys[i][0] == '\0') { kstrlcpy(keys[i], key, 8); vals[i][0] = '\0'; return i; }
    return -1;
}

void append(char*& p, const char* s) { while (*s != '\0') *p++ = *s++; }

void append_u(char*& p, uint32_t v)
{
    char t[12];
    int n = 0;
    do { t[n++] = static_cast<char>('0' + v % 10); v /= 10; } while (v != 0);
    while (n > 0) *p++ = t[--n];
}
}  // namespace

const char* sm_get(const char* key)
{
    int i = slot(key, false);
    return i < 0 ? "" : vals[i];
}

const Job& sm_job(uint32_t id) { return id >= 1 && id <= MAX_JOBS ? jobs[id] : none; }

bool sm_apply(uint32_t index, const Entry& e)
{
    switch (e.kind) {
    case K_SET: {
        int i = slot(e.key, true);
        if (i >= 0) kstrlcpy(vals[i], e.val, 8);
        return i >= 0;
    }
    case K_CAS: {
        int i = slot(e.key, true);
        if (i < 0 || kstrcmp(vals[i], e.expect) != 0) return false;
        kstrlcpy(vals[i], e.val, 8);
        return true;
    }
    case K_JOB_SUBMIT:
        if (e.a < 1 || e.a > MAX_JOBS || jobs[e.a].state != J_NONE) return false;
        jobs[e.a] = Job{J_SUBMITTED, 0, e.b, 0, 0};
        return true;
    case K_JOB_ASSIGN:
        if (e.a < 1 || e.a > MAX_JOBS || jobs[e.a].state == J_DONE) return false;
        jobs[e.a].state = J_ASSIGNED;
        jobs[e.a].node = static_cast<uint8_t>(e.b);
        jobs[e.a].assigned_at_index = index;
        return true;
    case K_JOB_DONE:
        if (e.a < 1 || e.a > MAX_JOBS || jobs[e.a].state == J_DONE) return false;
        jobs[e.a].state = J_DONE;
        jobs[e.a].result = e.b;
        return true;
    default:
        return true;
    }
}

void sm_format(const Entry& e, char* out)
{
    char* p = out;
    switch (e.kind) {
    case K_NOOP: append(p, "NOOP"); break;
    case K_SET: append(p, "SET "); append(p, e.key); append(p, "="); append(p, e.val); break;
    case K_CAS:
        append(p, "CAS "); append(p, e.key); append(p, " '"); append(p, e.expect);
        append(p, "'->"); append(p, e.val);
        break;
    case K_JOB_SUBMIT: append(p, "JOB_SUBMIT job "); append_u(p, e.a); append(p, " ("); append_u(p, e.b); append(p, " ticks)"); break;
    case K_JOB_ASSIGN: append(p, "JOB_ASSIGN job "); append_u(p, e.a); append(p, " -> node "); append_u(p, e.b); break;
    case K_JOB_DONE: append(p, "JOB_DONE job "); append_u(p, e.a); append(p, " result "); append_u(p, e.b); break;
    default: append(p, "?"); break;
    }
    *p = '\0';
}

void sm_dump(const char* why)
{
    kprintf("[t=%06u n%u] state (%s):", now(), g_node, why);
    for (int i = 0; i < NKEYS; ++i)
        if (keys[i][0] != '\0') kprintf(" %s=%s", keys[i], vals[i][0] != '\0' ? vals[i] : "''");
    for (uint32_t j = 1; j <= MAX_JOBS; ++j) {
        const Job& x = jobs[j];
        if (x.state == J_NONE) continue;
        static const char* names[] = {"none", "submitted", "assigned", "done"};
        kprintf(" job%u=%s", j, names[x.state]);
        if (x.state == J_ASSIGNED) kprintf("@n%u", x.node);
        if (x.state == J_DONE) kprintf("(%u)", x.result);
    }
    kprintf("\n");
}

int sm_export(char k[][8], char v[][8])
{
    int n = 0;
    for (int i = 0; i < NKEYS; ++i)
        if (keys[i][0] != '\0') { kstrlcpy(k[n], keys[i], 8); kstrlcpy(v[n], vals[i], 8); ++n; }
    return n;
}

void sm_restore(const char k[][8], const char v[][8], int n)
{
    for (int i = 0; i < n; ++i) {
        int s = slot(k[i], true);
        if (s < 0) continue;
        if (kstrcmp(vals[s], v[i]) != 0)
            klog("naive: adopting leader's %s=%s (mine was %s)", k[i], v[i], vals[s][0] != '\0' ? vals[s] : "''");
        kstrlcpy(vals[s], v[i], 8);
    }
}
