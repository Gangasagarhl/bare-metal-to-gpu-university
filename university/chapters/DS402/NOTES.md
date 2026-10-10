# DS402 — Distributed storage and observability: author notes

DS402 has five chapters, F5-39 to F5-43, at level L4. The prerequisite is DS302.

The analogy world is F5, "friends in different towns". The characters are Kofi, Mei, Sofia, Arjun and Leila, plus Omar in the F5-40 forensic lab.

Labs live in `university/labs/F5-39` to `university/labs/F5-43`.
- Every folder passes `university/labs/run_lab.sh university/labs/<ID>` with exit status 0.
- The last run of all five folders was on 2026-10-09, between 23:02 and 23:03 UTC.
- After those runs, no listing changed.

## Toolchain (as recorded in the logs)

- **Compiler:** `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
- **Flags:** `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined` (the `run_lab.sh` defaults).
- **Python:** 3.13.16, used only for F5-42 `dashcheck.py`, run as `python3 -I`, standard library only.
- **Machine:** Linux x86_64 cloud build container.
- **Isolation:** no network, threads, disks or external libraries. Every node, disk and client is an object or a number inside one process.
- **Determinism:** seeds are fixed, so every output is reproducible.
- **F5-42 `run.sh`:** it runs after the C++ listings. It writes `dashcheck.*`, a parser check of the generated HTML report, and `dashboard_head.*`, a trimmed copy of the report with the trim marked.

## Listings run

Nothing ran on real distributed hardware. "Untested on hardware" applies to every chapter, and each chapter has an unverified box saying so.

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F5-39 | gfs (GFS-style master, chunk servers, leases, record append, re-replication, stale replica on rejoin) | 0 | pass |
| F5-39 | wordcount (MapReduce word count, combiner, locality-aware scheduling, worker failure) | 0 | pass |
| F5-39 | stale (forensic evidence "the report that went back in time") | 0 | pass |
| F5-40 | ring (modulo vs consistent hashing, 1/64/256 tokens, preference lists) | 0 | pass |
| F5-40 | quorum (stale-read rates and waits for every (R, W) with N = 3; sloppy quorum print) | 0 | pass |
| F5-40 | resurrect (forensic evidence "the stove came back": vector clocks, union merge) | 0 | pass |
| F5-41 | service (instrumented KV service model: structured logs, counters, histogram, traces, sampling, cardinality) | 0 | pass |
| F5-41 | spike (course forensic "Latency spike at 14:02": pause on node a) | 0 | pass |
| F5-42 | slo (SLO, error budget, five alert rules replayed over a month) | 0 | pass |
| F5-42 | dashboard (static HTML SLO report, no JavaScript) and dashboard_head (trimmed copy) | 0 | pass |
| F5-42 | dashcheck (html.parser check of the report: balanced, no script, 30 bars, 31 rows) | 0 | pass |
| F5-42 | budget (forensic evidence "the budget is gone and nobody was paged") | 0 | pass |
| F5-42 | budget_key (answer-key calculation for the same month) | 0 | pass |
| F5-43 | chaos (six fault-injection experiments on a fluid model) | 0 | pass (two experiments abort by design; that is printed output, not a failure) |
| F5-43 | gameday (forensic evidence "the game day that took the service down") | 0 | pass |

There are no expected-fail listings.

### Scratch runs used in the answer keys (not in the lab logs)

All scratch runs were in `/tmp/claude-0/work/DS402/scratch`. Each answer that uses one says it is a scratch run.

- **F5-39 Check yourself 7:** a record-append forward that fails to cs1.
- **F5-40 Check yourself 7:** the delay distribution changed. Stale reads for (1,1) moved to 55.91 %, and the R = 3 / W = 3 waits fell to about 4.2.
- **F5-41 Check yourself 6:** printed p50 0.57, p99 10.81 and p99.9 12.33.
- **F5-42 Check yourself 7:** background failures raised to 0.05 %.
  - Budget used was 109.4 %.
  - Rule A: 811 alerts, 796 of them noise.
  - Rule E: 3 alerts, 1 of them noise, and it detected the slow burn after 919 min.
- **F5-43 Check yourself 5 and 6, at 1,500/s:**
  - E3's policy collapses: 2.67 % success, 4,435 attempts/s.
  - Killing r2 collapses too: 1.67 % success, 191 % load.

## Unverified boxes (per chapter)

- **F5-39:**
  - The GFS paper's concrete values: 64 MB chunks, 64 KB checksum blocks, the 60 s lease and the replica count, as recalled. HDFS's corresponding names and defaults.
  - Untested on real hardware.
- **F5-40:**
  - The Dynamo paper's production values: (3,2,2), latency figures and clock truncation, as recalled.
  - Untested on real hardware.
- **F5-41:**
  - Real telemetry data models and formats (OpenTelemetry, monitoring exposition formats). The listing's text format is illustrative only.
  - Untested on real hardware.
  - A "Watch out" box says lab steps 2–6 are untested.
- **F5-42:**
  - The Workbook's multiwindow, multi-burn-rate recommendation: 2 %/1 h/5 min, 5 %/6 h/30 min and 10 %/3 d/6 h. The thresholds 14.4, 6 and 1 are derived by arithmetic. The choice of fractions and windows is recalled.
  - Untested on real hardware. The report was checked by a parser and never opened in a browser.
- **F5-43:**
  - The chaos-engineering principles (D10), the metastable-failure definitions (D11) and the cascading-failure defences (D5), all recalled.
  - Untested on real hardware. The descriptions of kernel network-emulation and block-delay facilities are general. The claim that TCP handshakes complete while the application is blocked was not tested.
  - A "Watch out" box says lab steps 4–6 are untested.
  - A safety box covers fault injection that can cut off your own access.

## Sources

Every source is title-only and was not opened (dossier gate G1 open). Source numbers are local to each chapter. D7 and D8 mean different works in different chapters, but each chapter's Sources list is self-contained:

| Tag | Chapter | Work |
|---|---|---|
| D7 | F5-40 | Karger et al. |
| D7 | F5-41 | Dapper |
| D8 | F5-40 | Shapiro et al. |
| D8 | F5-42, F5-43 | the SRE Workbook |

### From the course card / F5 registry

- D1 Kleppmann, "Designing Data-Intensive Applications".
- D2 GFS (SOSP 2003).
- D3 MapReduce (OSDI 2004).
- D4 Dynamo (SOSP 2007).
- D5 "Site Reliability Engineering".
- D6 van Steen and Tanenbaum, "Distributed Systems".

### Proposed additions (not in the guide's F5 registry; the Source Researcher should confirm or replace them)

- Karger et al., "Consistent Hashing and Random Trees" (STOC 1997). F5-40, D7.
- Shapiro, Preguiça, Baquero, Zawirski, "Conflict-free Replicated Data Types" (2011). F5-40, D8.
- Sigelman et al., "Dapper, a Large-Scale Distributed Systems Tracing Infrastructure" (Google technical report, 2010). F5-41, D7.
- Dean and Barroso, "The Tail at Scale" (CACM 2013). F5-41, D9.
- Beyer, Murphy, Rensin, Kawahara, Thorne (eds.), "The Site Reliability Workbook". F5-42 and F5-43, D8.
- Basiri et al., "Chaos Engineering" (IEEE Software 2016). F5-43, D10.
- Bronson, Aghayev, Charapko, Zhu, "Metastable Failures in Distributed Systems" (HotOS 2021). F5-43, D11.
- OpenTelemetry specification, mentioned only in F5-41's unverified box. Not cited as a claim source.
- HDFS architecture documentation, mentioned only in F5-39's unverified box. Not cited.

## Proposed analogy mappings (F5 extensions, for the analogy registry)

- **F5-39:** the club library spread over friends' shelves = a distributed file system. The catalogue keeper = the master. A book's volumes = chunks. The keeper's permission slip = the lease. The edition number = the chunk version.
- **F5-40:** the wheel of letters (houses placed on a circle; a key goes to the next house clockwise) = consistent hashing. Several seats per house on the wheel = virtual nodes. Asking enough friends = quorums. Each friend's tally of edits = a vector clock.
- **F5-41:** diaries = logs. The tally sheet on the fridge = metrics. Tracking slips stamped at every stop = traces. The slip number = trace id. Stamping every tenth letter = sampling. The afternoon a house stood still = a stop-the-world pause.
- **F5-42:** the club's promise ("999 of 1,000 letters within a week") = the SLO. The allowance card = the error budget. The bell = an alert. Ringing at night versus a note on the fridge = a page versus a ticket.
- **F5-43:** the fire drill = a fault-injection experiment or game day. Leila staying home and sitting on letters = a slow (gray) failure. Resending to anyone = retries. "Resend at most one in ten" = a retry budget. The jam that outlasts Leila's illness = a metastable failure.

## Decisions for the owner

1. **"Instrument the DS302 service" lab.** The learner's own DS302 service does not exist in the repository, so F5-41 lab steps 2–6 and F5-43 lab steps 4–6 are marked untested. The DS302 simulator labs (F5-18/F5-19 Raft) do exist. A future build could instrument that simulator directly, which would make the course lab fully tested. Please decide whether to do that.
2. **Chapter-local D-numbers.** D7 and D8 mean different works in different chapters (see Sources). If the owner wants course-wide numbering, renumber them when the registry is updated.
3. **Seven proposed sources.** They should be confirmed, added to the F5 registry, or replaced. See the proposed additions above.
4. **Lines longer than 100 characters.** These are style only; every listing compiles with `-Werror`. They are in `F5-41/service.cpp` (line 43 among others), `F5-39/gfs.cpp`, `stale.cpp`, `wordcount.cpp`, `F5-40/quorum.cpp` and some F5-42/F5-43 files. Decide whether to enforce a column limit.
5. **No curriculum milestone.** No curriculum milestone or C-number maps to DS402 in the course card. "Maps to" names only the papers and the SRE book.
6. **Course project "Observability for MP2".** It is split across the chapters: part 1 in F5-41, part 2 in F5-42 and part 3 in F5-43. The practical exam ("write an SLO and alert") is practised in F5-42's mini-project.
7. **F5-43's "Next chapter" link.** It points to F5-44 (DS403). That link resolves now because DS403's F5-44 exists in the build.
