// sm.h - DS403 cluster kernel: the replicated state machine of F5-47.
// Every node applies the same commands in the same order and so reaches the same state:
// a small key-value table (with compare-and-set for locks) and the job table of the scheduler.
#pragma once
#include <stdint.h>

enum Kind : uint8_t { K_NOOP = 0, K_SET, K_CAS, K_JOB_SUBMIT, K_JOB_ASSIGN, K_JOB_DONE };

struct Entry {
    uint32_t term;        // the leader's term when the entry was created (Raft); 0 in naive mode
    uint8_t kind;         // Kind
    uint8_t client;       // node that asked for it (0 = the leader itself)
    uint16_t reqid;       // the client's request number
    uint32_t a, b;        // job id, node, duration or result
    char key[8], expect[8], val[8];
};

enum JobState : uint8_t { J_NONE, J_SUBMITTED, J_ASSIGNED, J_DONE };
struct Job { JobState state; uint8_t node; uint32_t duration, result, assigned_at_index; };
constexpr int MAX_JOBS = 8;

bool sm_apply(uint32_t index, const Entry& e);   // returns the command's result (CAS: swapped?)
const Job& sm_job(uint32_t id);
const char* sm_get(const char* key);
void sm_format(const Entry& e, char* out);         // "CAS lock ''->n3"
void sm_dump(const char* why);                     // print the whole state
void sm_restore(const char keys[][8], const char vals[][8], int n);   // naive mode only
int sm_export(char keys[][8], char vals[][8]);                         // naive mode only
