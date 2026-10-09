# DS302 — Consensus: author notes

Chapters F5-15 to F5-21, levels L3–L4, 4 credits. Prerequisite: DS301. Analogy world F5, "friends in different towns". The characters are the ones F5-15 introduces: Kofi, Amara and Mei, ordering pizza together. F5-17 adds Hana, who runs a café.

Labs are in `university/labs/F5-15` to `university/labs/F5-21`. Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. The last run of every folder was on 2026-10-09, between 22:22 and 22:25 UTC. No source file was changed after those runs.

## Toolchain (as recorded in the logs)

- **Compiler:** `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
- **Flags:** `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined` (the run_lab.sh defaults).
- **Machine:** Linux x86_64 cloud build container.
- **External libraries:** none. There is no real networking and no threads. Every "server" is an object inside one deterministic simulator.
- **Randomness:** splitmix64 with fixed seeds (`sim.h`). Every output is reproducible byte for byte; no output depends on wall-clock time.
- **Run time:** `F5-21` has `.timeout` files, `fuzz.timeout` 120 s and `bug_hunt.timeout` 300 s. Under the sanitizers the whole folder ran in about 70 s.

## Listings run

Nothing ran on real distributed hardware, and nothing in DS302 needs a GPU or any special device. "Untested on hardware" applies to the whole course in this sense: disks, clocks and networks are simulated.

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F5-15 | pizza (three voting rules, 1000 seeds each) | 0 | pass |
| F5-16 | flp_explore (exhaustive exploration of delivery orders) | 0 | pass |
| F5-17 | paxos_basic | 0 | pass |
| F5-17 | duel (dueling proposers, 100 seeds × 4 settings) | 0 | pass |
| F5-17 | two_values (forensic evidence: acceptor compares with the wrong ballot) | 1 | **expected fail**: the checker reports two chosen values |
| F5-17 | two_values_fixed | 0 | pass |
| F5-18 | election (course lab: Raft with a deterministic network simulator) | 0 | pass |
| F5-18 | split_vote (200 seeds × 3 settings) | 0 | pass |
| F5-18 | two_leaders (forensic evidence: lazyPersist) | 1 | **expected fail**: Election Safety and other violations |
| F5-18 | two_leaders_fixed | 0 | pass |
| F5-19 | replication (partitions, a lagging follower, a deposed leader) | 0 | pass |
| F5-19 | divergence (forensic evidence: skipPrevLogCheck) | 1 | **expected fail**: State Machine Safety and Log Matching |
| F5-20 | committed_lost (course forensic "Committed entry lost": commitOldTermByCount, Figure 8 of the Raft paper) | 1 | **expected fail**: Leader Completeness and State Machine Safety |
| F5-20 | figure8_fixed | 0 | pass |
| F5-20 | joint (membership change by joint consensus, including removing the leader) | 0 | pass |
| F5-20 | quorums (exhaustive count of disjoint majorities) | 0 | pass |
| F5-21 | lin_demo (linearizability checker on 4 histories) | 0 | pass |
| F5-21 | fuzz (correct Raft, 100 random fault schedules) | 0 | pass: 0 of 100 failed |
| F5-21 | bug_hunt (each planted fault under fault injection) | 0 | pass (catch rates are output, not failures) |
| F5-21 | stale_read (forensic evidence: readFromLocalState) | 1 | **expected fail**: the history is not linearizable |
| F5-21 | stale_read_fixed | 0 | pass |

There are five expected-fail runs. In each one, the program's own checker detects the planted bug and exits 1, and each has a fixed counterpart that exits 0. Every chapter's R1 source entry states the exit codes.

### Scratch runs quoted in answers (not in the lab logs)

- **F5-20, check-yourself answer 7:** `figure8_story.h` with `noopOnElection = true`, with the fault kept on. The chapter says plainly that this was a scratch run made by the author. Result: n5 commits a term-2 no-op at 480 ms, which replaces n2's uncommitted x=2. n1 is refused in every later election. The final states are x=3 y=4, with no safety violation.

### Things found while building the labs (useful for the owner and for the exam)

These bugs were found in the lab library itself while it was being built, before the final runs. All are fixed, and the chapters mention the ones that teach something.

- **A deposed leader started an election at once:** its old election deadline was already in the past. Fixed by resetting the timer in `becomeFollower`; F5-18 mentions this.
- **Duplicate client replies after a restart.** Fixed: only the leader of the entry's own term answers.
- **Scripted `reconnect` re-linked servers that were still isolated.** Fixed with the `isolated_` set in `cluster.h`.
- **The linearizability checker blew up on one seed** (readFromLocalState, seed 27). Fixed with two changes, both explained in F5-21:
  - memoization on (placed set, value);
  - dropping unanswered puts whose value no get ever returned.
- **The Figure 8 fault is almost invisible to random fault injection** (0 of 100 seeds). The leader-hunter nemesis was written to find it (2 of 150). This is an honest result, and F5-20 and F5-21 both state it.
- **The Paxos duel (F5-17) needs an explanation.** At a 14 ms timeout, even a single proposer never learns the value, because 14 ms is shorter than one complete round. The chapter includes the one-proposer control row and says so.

## Unverified and untested boxes

- **F5-15** (2 boxes):
  - Byzantine fault tolerance (3f+1) and uniform agreement are named but not taught. Their sources are not in the registry.
  - Device flush/fsync semantics, that is, whether a "durable" write survives a power cut, are not verified. Nothing was tested on real disks.
- **F5-16** (1 box): partial synchrony (Dwork–Lynch–Stockmeyer), randomized consensus (Ben-Or) and failure detectors (Chandra–Toueg) are named but are not in the registry.
- **F5-17** (1 box): "The Part-Time Parliament", later Paxos variants and how production systems relate to Multi-Paxos were not consulted.
- **F5-18** (1 box): pre-vote and "ignore RequestVote while a leader is alive" are not verified, and the lab implements neither:
  - both are in Ongaro's dissertation, which is not in the registry;
  - whether the paper itself describes the second remedy is to be confirmed.
- **F5-19** (1 box): the claim that a leader may write to its own disk in parallel with replication is recalled from the dissertation, and the paper is to be confirmed. The simulated disk is instantaneous.
- **F5-20** (1 box): single-server membership changes and their published correction are not taught. The claim that new servers join as non-voting members first must be confirmed in the paper.
- **F5-21** (1 box): these are recalled, not sourced:
  - Herlihy–Wing (the definition of linearizability);
  - Wing–Gong (the search);
  - Jepsen;
  - the TLA+ specification of Raft;
  - NP-completeness of linearizability checking.
- **Sources (all chapters):** every D-source is "title only — not opened during this build" (gate G1). Section and figure numbers of the Raft paper appear in lab code comments and in a few chapter sentences ("Figure 2", "Figure 3", "Figure 8"). They are recalled and must be confirmed.
- **"How the hardware actually does it":** these sections give no numbers. They point to measurement (F5-07 methods).

## Decisions for the owner

1. **Course card versus curriculum.** SYSTEMS_CURRICULUM has no consensus milestone, so DS302 was written from the course card and guide 9.2 alone. A milestone needs to be added, or the mapping confirmed.
2. **Source registry additions** (none were added in this build). Each is needed to close an unverified box:
   - Ongaro's PhD dissertation (pre-vote, single-server changes, parallel leader writes);
   - Herlihy & Wing, "Linearizability" (DS301 also proposes it as D8 of F5-13);
   - Wing & Gong (linearizability checking);
   - Dwork, Lynch & Stockmeyer (partial synchrony);
   - Ben-Or (randomized consensus);
   - Chandra & Toueg (failure detectors);
   - optionally Lamport's "The Part-Time Parliament" and the Raft TLA+ specification.
3. **Proposed new analogy mappings** (not in the registered F5 list; please accept or replace):
   - ballot / proposal number = the number printed on a café order form; promise = "I will ignore forms with smaller numbers" (F5-17);
   - term = a numbered "round of being in charge" (F5-18);
   - membership change = joining or leaving the group chat (F5-20);
   - fault injection / nemesis = a deliberately bad mail service (F5-21);
   - linearizability = all answers fit one notebook read in one order, consistent with the clock (F5-21).
4. **One Raft library, four identical copies.** The Raft library is copied byte for byte into `labs/F5-18` … `labs/F5-21`, because each lab folder must build alone. The copied files are `raft.h`, `raft_election.h`, `raft_replication.h`, `raft_membership.h`, `cluster.h` and `sim.h`. `sim.h` is also identical in F5-15 and F5-17. Any fix must be applied to all copies; their md5 sums matched at the end of this build. The owner may prefer a shared include directory, if run_lab.sh is changed to allow one.
5. **Deliberate faults in the shipped library.** `raft::Faults` holds the four forensic bugs (`lazyPersist`, `skipPrevLogCheck`, `commitOldTermByCount`, `readFromLocalState`), all off by default.
   - This makes the forensic labs and the planted-bug table possible.
   - It also reveals the fault names to a student who reads `raft.h` before a forensic lab. The forensic questions ask which fault matches the evidence, so the names act as hypotheses, not answers. The owner may still prefer to hide them.
6. **Course project = MP2 seed.** The F5-21 mini-project is the course project, a Raft-replicated key-value service. Its rubric demands an honest planted-bug table. The exam item "P: find the bug in a provided Raft trace" can reuse any of the five forensic traces, or new ones generated by switching on a different `Faults` flag with another seed.
7. **Shared glossary terms with DS301.** "Linearizability", "Stale read" and "History (of operations)" are copied verbatim from DS301's glossary. Only the `chapters` field differs, and build.py merges the chapters. All other DS302 terms are new.
8. **Lab reads go through the log.** For linearizable reads, the lab sends gets through the log. ReadIndex and leases are described in F5-21 but not implemented; this choice is left for MP2.
