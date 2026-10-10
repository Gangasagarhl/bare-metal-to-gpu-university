# MP2 — A small distributed OS service layer with consensus: author notes

One handbook, `MP2.html` (front matter `id: MP2`, `course: MP`, level L5), a starter lab in
`university/labs/MP2/`, and eight new glossary terms in `glossary.json`. Card: guide 11.6
(prereqs DS302, DS402, DS403). Analogy world F5 (friends in different towns: Kofi, Amara,
Mei), one short paragraph as the brief asks for L5. Proposed F5 mappings (not in the
registry; accept or replace), used only in the Jargon box and Layer 1: fault schedule = a
fire-drill plan saying which door is locked at which minute (extends DS402's fire drill =
game day); planted-bug table = known mistakes hidden in an exam to test the marker; run
fingerprint = a photograph of the finished notebook pages; liveness probe = posting one
letter after the storm; election storm = friends calling new votes because the organiser's
postcard arrives late; protocol specification = the club's rulebook; client session =
numbered letters so duplicates can be thrown away; zombie worker = a friend cut off by a
storm who keeps doing a chore already given to someone else.

## Files

- `university/chapters/MP2/MP2.html` — the handbook.
- `university/chapters/MP2/glossary.json` — new terms only: Fault schedule (scenario), Catch
  rate (planted-bug table), Run fingerprint, Liveness probe (after the faults stop), Election
  storm, Protocol specification (written), Client session (duplicate detection), Zombie worker.
  About 30 existing terms are linked from the Glossary links section (all resolved at build).
- `university/labs/MP2/` — the starter lab (milestone 1 in miniature):
  - copied byte for byte from `labs/F5-21` (md5 sums equal at the end of this build):
    `sim.h`, `raft.h`, `raft_election.h`, `raft_replication.h`, `raft_membership.h`,
    `cluster.h`, `linearizability.h` — the learner's own DS302 library, unchanged;
  - new: `harness.h` (fault-schedule language, `Observed` cluster with election metrics,
    four checks: safety, linearizability, liveness after settle, determinism by double run
    and fingerprint), `suite.cpp` + `suite.in` (9 scenarios, 67 runs), `mutants.cpp`
    (planted-bug table), `replay.cpp` (replay of one run with trace window), `incident.h`
    + `incident.cpp` + `incident_fixed.cpp` (forensic evidence and its key);
  - `suite.timeout` 120 s, `mutants.timeout` 240 s.

## Toolchain and runs

`university/labs/run_lab.sh university/labs/MP2` from the repository root, exit status 0.
Last run 2026-10-10 about 03:10 UTC; the rerun produced byte-identical `.out` files (the
simulator is deterministic). g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, run_lab.sh default
flags (C++20, -Werror, ASan + UBSan). Whole folder about 70 s, most of it `mutants`.

| Listing | Run | Exit | Status |
|---|---|---|---|
| suite.cpp (stdin suite.in) | 9 scenarios, 67 runs each executed twice | 0 | pass: 67 runs, 0 failed, all deterministic |
| mutants.cpp | 7 variants × 67 runs | 0 | pass (catch rates are output): no false alarm; lazyPersist, skipPrevLogCheck, readFromLocalState, commitOldTermByCount-without-no-op killed; **commitOldTermByCount (no-op on) and "heartbeat 200 ms" SURVIVED** — reported honestly, both are R1 tasks |
| replay.cpp | leader-isolated, seed 12, readFromLocalState | 0 | pass: shows the stale read (get x -> 8 at 1312 ms after put x=13 answered at 1236 ms), identical fingerprint twice |
| incident.cpp | forensic evidence, heartbeat 200, seed 21 | 0 | pass: term 1 → 14 with no faults, SLO 13/17 = 76.5 % missed, checkers green |
| incident_fixed.cpp | same run, default heartbeat 50 | 0 | pass: term stays 1, SLO 17/17 met |

No expected-fail listings. Everything is **simulated**; nothing ran on hardware, a real
network, a second machine, a VM or a disk. All latencies are simulated milliseconds and the
handbook says so wherever they appear.

### Scratch runs quoted in the handbook (not in the lab logs)

- Check yourself answer 5: `leader-isolated` with the heal moved from 1500 to 600 ms, variant
  readFromLocalState, seed 12 → passes all four checks (bug not caught). The answer says it is
  a scratch run and gives only a "plausible reading" of why.
- While writing: the whole suite with heartbeat 200 → 0 failures in all 67 runs; this is now
  a real lab result (the "heartbeat 200 ms" row of `mutants.out`), not only a scratch run.

### Things found while building the lab (used in "Common mistakes")

- First suite version had three clients; they started all 50 operations by about 900 ms,
  so faults scheduled at 800–2000 ms hit an idle system. Fixed: two clients, faults 400–1300 ms.
- First forensic version used continuous client traffic: appends reset follower timers, so
  the heartbeat misconfiguration produced no extra elections until traffic stopped. Fixed with
  a light paced load (one request per 200 ms). Both are stated in the handbook.

## Unverified boxes in MP2.html

1. Layer 3: what a flush guarantees on real disks, virtual disks and file systems (NVMe flush,
   POSIX fsync) — not checked; same open question as DS302 F5-15.
2. Layer 3: read index / lease reads, pre-vote, check-quorum, single-server membership changes
   and their correction, non-voting catch-up — from memory of Ongaro's dissertation (not in
   the registry); not implemented or tested.
3. Layer 3: Raft's timing requirement ("broadcast time ≪ election timeout ≪ MTBF") and its
   section in the paper — from memory.
4. Milestone 4: no QEMU-monitor, netem/tc, packet-filter or hypervisor command lines are
   given; milestone 4 is untested on hardware and untested in this build (no VM manager, no
   second machine).
5. Hardware section: untested on hardware (whole project).

All D-sources are title only, not opened (gate G1 open). D6 (the guide) and D7 (the
curriculum, C9 acceptance tests) are the project's own documents, quoted for scope only.

## Decisions for the owner

1. **Template adapted to a project.** All 21 template headings are present in order, plus six
   project sections as extra `<h2>`s: Milestones, Review gates R0–R4, Deliverables, Grading
   rubric, Risks and safety (after Layer 3), and "Incident and forensic write-up template"
   (after the Forensic lab). The Mini-project section holds the "starter extensions due at R1".
   build.py accepted this (no PROBLEM lines). Please confirm this shape for MP1–MP8, so the
   eight handbooks look alike.
2. **Level.** The card gives none; L5 chosen (after L4 courses DS402 and DS403, L4–L5).
3. **Gate checklists, signers and rubric level descriptors** are this handbook's proposal; the
   card fixes only the gate positions and the weights (kept exactly: 35/20/15/15/15). The
   guide says the owner decides who signs; the handbook asks for independent peers, a mentor
   where available, and the owner's named supervisor for R3 safety items.
4. **Seed counts.** Milestone 1 asks for at least 1,000 generated seeds per nemesis kind
   (as DS302's course project did); the starter uses 30 to stay within the lab time limit.
5. **The forensic lab reveals its root cause in `mutants.out`.** The "heartbeat 200 ms" row
   of the planted-bug table (shown in the Code walk-through, before the forensic lab) is the
   same misconfiguration. Kept on purpose: it shows the suite would not have caught the
   incident. The owner may prefer to move that row to the answer key.
6. **Library copied again (fifth copy).** As DS302 decided, each lab folder builds alone, so
   the DS302 Raft library now exists in `labs/F5-18` … `F5-21` and `labs/MP2`. A fix must be
   applied to all copies, or run_lab.sh could allow a shared include directory.
7. **Milestone 4 fallback.** The handbook requires a written decision record approved at R2
   before the Linux-VM fallback is used, and the C9 acceptance tests (quoted from the
   curriculum) as the entry criterion for the own-OS path.
8. **Source registry additions** (same as DS302/DS402/DS403 notes): Ongaro's dissertation,
   the SRE Workbook (D4), the Borg paper (D5).
9. **MP1 link.** The handbook links `#MP1` (the card or, when written, the MP1 handbook) and
   assumes MP1 delivers a kernel with C9 networking and a block driver; check against the
   MP1 handbook when both exist.
