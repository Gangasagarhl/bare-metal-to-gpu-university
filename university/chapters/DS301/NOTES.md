# DS301 — Distributed systems fundamentals: author notes

Chapters F5-08 to F5-14, level L3, 4 credits. Prerequisites: DS201, SP203. Analogy world F5, "friends in different towns". The characters are Amara, Bao and Chidi, who run a book-swap club across three towns; Dara joins in F5-14.
Labs are in `university/labs/F5-08` to `university/labs/F5-14`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All seven were run again, one after the other, at the end of this build on 2026-10-09, after the last code change.

## Toolchain (as recorded in the logs)

- **Compiler:** `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
- **Flags:** `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined` (the run_lab.sh defaults).
- **Machine:** Linux x86_64 cloud build container.
- **External libraries:** none. The only network use is TCP over the loopback interface (F5-09).
- **Randomness:** every program that uses random numbers uses a fixed-seed splitmix64 generator, so its output is reproducible.
  - The only output that depends on timing is `F5-10/clocks.out`, and the chapter quotes only its resolution and "never went backwards" lines.
  - The F5-09 RPC runs depend on scheduling only through timeouts that are chosen far apart, and their recorded outcomes were stable across reruns.

## Listings run

Nothing ran on real distributed hardware: all "nodes" are threads or variables in one process, and all networks are loopback or simulated.

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F5-08 | availability | 0 | pass |
| F5-08 | fanout | 0 | pass |
| F5-08 | outages (forensic evidence) | 0 | pass |
| F5-09 | rpc_demo (RPC library over TCP loopback: course lab) | 0 | pass; untested across a real network |
| F5-09 | retry | 0 | pass; untested across a real network |
| F5-09 | reconnect (forensic evidence) | 0 | pass; untested across a real network |
| F5-10 | clocks | 0 | pass (timing-dependent values not quoted) |
| F5-10 | lamport (three-node simulation: course lab) | 0 | pass |
| F5-10 | vclock | 0 | pass |
| F5-10 | two_leaders (forensic evidence "Two leaders": course forensic) | 0 | pass |
| F5-10 | two_leaders_truth (answer-key truth) | 0 | pass |
| F5-11 | detector | 0 | pass |
| F5-11 | flapping (forensic evidence) | 0 | pass |
| F5-12 | counter (replicated counter that fails in instructive ways: course lab) | 0 | pass (the WRONG rows are intended output, not failures) |
| F5-12 | failover (forensic evidence) | 0 | pass |
| F5-13 | histories (stdin `histories.in`) | 0 | pass |
| F5-13 | quorum | 0 | pass |
| F5-13 | vanishing (forensic evidence) | 0 | pass |
| F5-13 | vanishing_check (by `run.sh`: checker on the forensic history) | 0 | pass |
| F5-14 | sharding | 0 | pass |
| F5-14 | hotshard (forensic evidence) | 0 | pass |

There are no expected-fail runs. The programs that show failures (wrong counts, lost writes, two leaders) print them as their normal output and exit 0.

### Scratch runs quoted in answers (not in the lab logs, labelled as such in the chapters)

- **F5-09, answer 6:** the retry variant described in that answer.
- **F5-11, answer 6:** the detector without the 20.0–20.5 s stall. False suspicions for T = 150/200/300/500/700/1000 ms were 9/4/0/0/0/0.
- **F5-12, answer 6:** batch size 5 instead of 8. Row 5 prints 100, with 0 acknowledged increments lost.
- **F5-13, answer 5:** history H8 is neither linearizable nor sequentially consistent.
- **F5-14, answer 5:** hot share 0.05 instead of 0.35. At 12:01 shard 2 has 10,123 requests (17 %); the other shards have 7,008–7,298.

## Unverified and untested boxes

- **F5-08:** a = 0.99 and p = 0.01 are exercise values, not measurements. No product's availability is quoted.
- **F5-09:** no production RPC framework or IDL is described. Untested on a real network: loopback only, and faults are injected by the program.
- **F5-10:** no clock-synchronisation accuracy or oscillator drift is stated. Untested on real hardware: clock offsets are chosen by the simulator.
- **F5-11:** no product's timeout defaults or detector algorithms are given. Phi-accrual detection is named only as a topic. Untested on real hardware: heartbeats are simulated.
- **F5-12:** architectures only, with no product claims. The loss and duplication rates are exercise values. Untested on real hardware: no durable writes (fsync and device caches are pointed to OS304, OS401 and HW205), and the course project has only been run on one machine.
- **F5-13:** two statements come from memory of D1 (linearizability checking is NP-complete in general; the precise form of CAP), and no product consistency claims are made. Untested on real hardware: the histories are scripted.
- **F5-14:** two statements come from memory (FNV-1a's weak low bits; per-process randomised language hashes). Imbalance numbers are specific to this hash and these keys. Untested on real hardware: no data movement was measured.

## Sources

Every D-source is "title only — not opened during this build (dossier gate G1 open)".

| Tag | Source | Where used |
|---|---|---|
| D1 | Kleppmann, "Designing Data-Intensive Applications" | all chapters |
| D2 | van Steen and Tanenbaum, "Distributed Systems" | all chapters |
| D3 | Fischer, Lynch and Paterson (FLP) | F5-08, F5-11 |
| D3 | Lamport 1978, "Time, Clocks, and the Ordering of Events in a Distributed System" | F5-10 (per-chapter numbering) |
| D4 | POSIX (The Open Group Base Specifications) and Linux manual pages | F5-09 (sockets), F5-10 (clocks) |
| D5 | ISO/IEC 14882 (C++) working draft | F5-08 |
| D6 | DeCandia et al., Dynamo (SOSP 2007) | F5-10, F5-12, F5-13, F5-14 |
| D7 | Shapiro, Preguiça, Baquero and Zawirski, "Conflict-free Replicated Data Types" | F5-12 |
| D8 | Herlihy and Wing, "Linearizability: A Correctness Condition for Concurrent Objects" | F5-13 |

**Proposed additions to the F5 source registry.**

- **Cited now:** D7 and D8 are not in the guide's F5 registry. The chapters cite them marked "from the author's memory; Source Researcher must confirm title, venue and year".
- **Proposed for later revisions, not cited:**
  - Birrell and Nelson, "Implementing Remote Procedure Calls", for F5-09.
  - Chandra and Toueg, "Unreliable Failure Detectors for Reliable Distributed Systems", for F5-11.

## Analogy proposals (new F5 mappings, not yet registered)

- **F5-08:** why distribute = the club outgrows one friend's shelf, and friends cover for each other.
- **F5-10:** Lamport clock = numbering every letter, and jumping your own counter past any number you receive.
- **F5-11:** failure detector = deciding a friend is ill because no letter has come for a while.
- **F5-12:**
  - synchronous versus asynchronous replication = waiting for the "copied" postcard versus posting the day's bundle in the evening.
  - failover = Bao taking over after the flood.
  - G-counter = each friend counts only on her own line, and copies merge by keeping the larger number per line.
  - anti-entropy = swapping notebooks once a month.
- **F5-13:** consistency model = the club's promise about what a member can find in any copy of the notebook. Linearizability = "as if there were one notebook in the town square".
- **F5-14:**
  - shard = each town keeps its own range of letters of the catalogue.
  - consistent hashing = towns at points around a circle of numbers.
  - hot key = everyone asking about one book.

## Glossary

`glossary.json` has 57 entries, generated from the chapters' jargon boxes, so the wording matches the pages.

- **Defined elsewhere, so left out of this file:** Tail latency and Framing (defined in DS201) and Lost update (defined in SP203). The chapters' "Glossary links" point to the same anchors.
- **Renamed:** "Node" became "Node (distributed systems)", because HW101 defines Node in the circuit sense.
- **Linked, not redefined:** sequential consistency, memory consistency model, median and percentile, tail of a distribution, latency, throughput, monotonic and wall-clock time.

## Decisions for the owner

1. **Forensic "Two leaders" (F5-10):** the root cause is a leader paused for 5 s plus skewed wall clocks, which lets a second leader be elected while the first still believes it leads. The logs come from three nodes with skewed clocks. The guide's F5 forensic idea, a Raft node that does not persist its term, is different. That idea fits DS302 (consensus) better and has not been used here.
2. **Course project placement:** the "primary-backup KV store with failure injection" brief is the F5-12 mini-project. F5-14 offers a sharded extension, and F5-09 and F5-11 supply the RPC library and the failure detector it builds on. No reference solution was written.
3. **Exams:** no exam or answer-key files were written; exams Q, M, F and P are not in the per-chapter template. The "order events from logs" practical can reuse `F5-10/two_leaders.out` and `F5-13/vanishing.out`.
4. **F5-13 `run.sh`:** the forensic answer key's checker run comes from an executable `run.sh` in the lab folder, which run_lab.sh calls after the normal runs. The checker's input is cut from `vanishing.out`, not copied by hand.
5. **Curriculum mapping:** DS301 has no curriculum milestones of its own (it maps to books). F5-11 points to the curriculum's 14.1 "Fault tolerance" row (`#trackF`).
6. **Links to unwritten courses:** links to DS302 (consensus, atomic commit) and to OS304, OS401 and HW205 resolve through catalogue anchors. `build.py` reported no problems for F5-08 to F5-14.
