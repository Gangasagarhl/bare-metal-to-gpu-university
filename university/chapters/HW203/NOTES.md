# HW203 The memory hierarchy — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; brief `/tmp/claude-0/prompts/HW203.txt`).
Level L2–L3, 4 credits. Faculty F1, analogy world: the restaurant building and its kitchen. Card "Maps
to: Curriculum 1.3, P4 (CS:APP ch. 6, 9)". Course card items and where they live:

- Lab "stride benchmark on own CPU (results, not spec numbers)": F1-32 (`chase`, `bandwidth`) and F1-34
  (`stride`, `traversal`), all labelled "measured on the build container" (AH-23).
- Lab "CS:APP cache lab": **not available in this build** (the authors' handout and grading files were not
  present and could not be fetched). F1-34 carries an unverified box saying so, and the course's own
  cache simulator (F1-33 `cache.hpp`) is used for the same exercises. Owner decision below.
- Forensic "Same work, ten times slower" (traversal order, explain from perf counters): F1-34's forensic
  lab. `perf` was unusable, so the counters come from **Cachegrind (simulated)**, clearly labelled.
- Exam style "predict cache behaviour of a loop nest, then measure": F1-34 `loopnest` + Check yourself.
- Project "cache simulator in C++ validated against hand traces": F1-33 `cache.hpp` / `cachesim.cpp`
  is the seed; each chapter's mini-project extends it (write policies F1-35, MESI F1-36, store buffer
  F1-37, TLB F1-38, DRAM back end F1-39).

## Files

- Chapters: `F1-32.html` … `F1-39.html` — all 21 template sections + Answers (forensic key as
  `<ID>-forensic-key` inside Answers), Jargon box, Transition box, at least one inline SVG figure each
  (F1-32 hierarchy pyramid "not to scale"; F1-33 address split + set-associative lookup; F1-36 MESI state
  diagram + false sharing; F1-37 store buffers + strong-to-weak scale; F1-38 4-level page-table walk +
  TLB reach; F1-39 DRAM organisation + NUMA).
- Glossary: `glossary.json`, 59 four-part entries generated from the Jargon boxes ("Set" and "Way"
  renamed "Set (cache)" and "Way (cache)" to match the links). Every `#gl-…` link in the eight chapters
  resolves; `build.py` reported no PROBLEM line for F1-32…F1-39.
- Labs: `university/labs/F1-32` … `F1-39`, each with `run.sh`.

## Listings run

Every lab passed `university/labs/run_lab.sh university/labs/<ID>` from the repository root. Toolchain
`g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`; models and simulators with `-std=c++20 -Wall -Wextra
-Wpedantic -Werror -g -fsanitize=address,undefined`; timing programs (`.cc`, built by `run.sh`) with
`-O2` and no sanitizers. Machine: Linux x86_64 KVM guest, 4 vCPUs, "Intel(R) Xeon(R) Processor @
2.10GHz", 1 NUMA node, THP mode `madvise`, page size 4096; kernel-reported caches L1d 48 KiB 12-way,
L2 2 MiB 16-way, L3 260 MiB 20-way (a VM's report; see F1-32 unverified box).

| Chapter | Runs (log names) | Result |
|---|---|---|
| F1-32 | `chase_random`, `chase_sequential`, `bandwidth`, `littles`, `littles_measured` | pass, exit 0; timings measured |
| F1-33 | `addr_split`, `cache_info`, `cachesim`, `cachesim_2way`, `cachesim_full`, `sameset`, `conflict` | pass, exit 0; `conflict` measured |
| F1-34 | `stride`, `traversal`, `loopnest`, `cachegrind_rows`, `cachegrind_cols`, `cachegrind_cols_bigLL`, `counters`, `perf_try` | pass; `perf_try.log` records exit code 2 — the evidence that `perf` is a wrapper without a matching binary ("perf not found for kernel 6.18.44-fc"); `counters` shows `perf_event_open` hardware events failing with ENOENT |
| F1-35 | `writepolicy`, `writeback_fixed`, `writeback_bug` (forensic: wrong memory contents by design, exit 0), `writes`, `writes_calls` | pass, exit 0 |
| F1-36 | `mesi`, `pingpong`, `padded`, `false_sharing`, `false_sharing_tsan` (no report) | pass, exit 0 |
| F1-37 | `litmus_model`, `store_buffer`, `peterson`, `peterson_asm_relacq`, `peterson_asm_seqcst` | pass, exit 0 |
| F1-38 | `vaddr_split`, `tlb_sim`, `first_touch`, `tlb_reach` | pass, exit 0 |
| F1-39 | `dram_model`, `threads_bw`, `numa_info` | pass, exit 0 |

No expected-fail listings (the buggy write-back simulator runs cleanly and prints wrong values; that is
the evidence). No CUDA/HIP.

**Untested on hardware:** hardware performance counters (no usable `perf`, no `perf_event_open`
hardware events — every "counter" number is Cachegrind's simulation); NUMA (one node; the F1-39 NUMA
lab step and the `numactl`/`libnuma` names are untested); Arm/RISC-V memory ordering (F1-37 ran on
x86-64 only); the CS:APP Cache Lab.

**Rerun-sensitive outputs — important.** The prose and answer keys quote the numbers of the runs in the
lab folders (e.g. F1-32 263.64 ns, F1-34 24.8 vs 217.2 ms, F1-36 2489.6 vs 283.7 ms, F1-37 724/916/0
store-buffering counts and 718 lost additions, F1-38 201.2 ms / 65,539 faults, F1-39 7.53 → 23.17 GB/s).
Rerunning `run.sh` regenerates `.out` files with different numbers, and the quoted figures would then
disagree with the displayed outputs. Timing-output files: `F1-32/{chase_*,bandwidth,littles_measured}`,
`F1-33/conflict`, `F1-34/{stride,traversal}`, `F1-35/writes`, `F1-36/false_sharing`,
`F1-37/{store_buffer,peterson}`, `F1-38/{first_touch,tlb_reach,vaddr_split (address only)}`,
`F1-39/threads_bw`. The deterministic ones (models, simulators, Cachegrind counts, objdump) are stable.
After the outputs were final, only one source comment was edited (`F1-38/first_touch.cc` line 1,
"twice" → "three times"); no behaviour changed.

## Claims that could not be verified (sources title only, gate G1 open)

No source document was opened. Source ids used in the chapters (each with "Title only — not opened
during this build; the Source Researcher must confirm the edition and section (dossier gate G1 open)"):
D1 CS:APP (ch. 6, 9, 12); D2 Patterson & Hennessy, COD; D3 Hennessy & Patterson, CA:AQA; D4 Nagarajan,
Sorin, Hill & Wood, "A Primer on Memory Consistency and Cache Coherence" (2nd ed.); D5 Harris & Harris;
D6 ISO/IEC 14882 (C++20 draft); D7 Intel SDM Vol. 3 (memory ordering, paging); D8 Linux kernel docs and
man-pages (THP, `madvise`, `getrusage`, `proc`, NUMA sysfs); D9 Valgrind manual (Cachegrind); D10
GCC/Clang ThreadSanitizer docs (F1-36). R-ids are this build's lab runs.

Unverified boxes (one per open question):

- F1-32: outstanding-miss capacity and prefetchers of the build container's CPU; why the latency
  curve climbs at a few MiB, far below the L3 size the VM reports.
- F1-33: replacement policy and inclusion of the real caches (the simulator uses true LRU).
- F1-34: perf event names for cache/TLB misses on a given CPU; CS:APP Cache Lab not available/untested.
- F1-35: whether glibc's large `memcpy` uses cache-bypassing stores on this CPU.
- F1-36: the CPU's actual coherence variant (MESIF/MOESI), interconnect, snoop filter/directory.
- F1-37: x86 = TSO per the vendor manual, and the ordering rules of Arm/RISC-V (only x86-64 tested).
- F1-38: TLB sizes/levels, page-walk caches, 5-level paging status, walk cost on this CPU.
- F1-39: XOR-hashed bank/channel mapping in real controllers, channel/rank/bank count of the host;
  separate "Untested on hardware — NUMA" box (`numactl`/`libnuma` named from memory).

Open measurement puzzles stated honestly in the text (not explained): F1-38 Listing 3's step at 128
blocks that appears with and without huge pages; spread+huge staying above packed (a set-conflict
derivation is offered as a candidate, not confirmed).

## Analogy mapping proposals (for the F1 analogy registry)

Registered mappings used: registers = hands/cutting board, caches = shelves near the station, DRAM =
pantry down the corridor, storage = warehouse, threads = cooks, atomic = tally counter. Proposed new
mappings (not yet registered — owner to accept or change):

| Concept | Proposed mapping | Chapter |
|---|---|---|
| Cache line | a tray: the shelf is always filled a whole tray at a time | F1-33 |
| Set / way | a shelf section reserved for certain items / a slot in that section | F1-33 |
| Dirty bit | a red sticker on a tray whose contents were changed | F1-35 |
| Write buffer | an outbox tray for items going back to the pantry | F1-35 |
| Coherence (MESI) | kitchen rule: many read-only recipe copies or one writable copy; "tear up your copies" | F1-36 |
| False sharing | two recipes printed on one sheet | F1-36 |
| Store buffer | each cook's outbox tray of notes not yet pinned on the shared board | F1-37 |
| Fence | "pin it up and wait before you look" | F1-37 |
| Virtual address / page table / TLB | ingredient name / the pantry index binder with four tabbed sections / sticky notes at the station | F1-38 |
| Page fault (first touch) | the manager fetches and clears a new box and writes it in the binder | F1-38 |
| DRAM bank / row buffer / channel | a pantry aisle with a clerk / the shelf pulled out onto the counter / a corridor with its own trolley | F1-39 |
| NUMA | two kitchens, each with its own pantry, joined by a hallway | F1-39 |

Note: F1-35's "write buffer = outbox tray" and F1-37's "store buffer = outbox tray of notes" are close;
the owner may want one image for both (they are related structures) or two distinct images.

## Decisions for the owner

1. **CS:APP Cache Lab.** The card names it; it was unavailable. Options: obtain the official handout
   and add it as an external lab in F1-34 (licence check needed), or accept the course simulator labs as
   the substitute.
2. **Counters.** The card asks to "explain from perf counters"; this build used Cachegrind (simulated)
   instead. A rerun of F1-34 (and F1-36/F1-38) on a machine with working `perf` should add real counter
   evidence and close the related unverified boxes.
3. **NUMA lab.** Needs a two-socket machine; until then the NUMA step stays "untested on hardware".
4. **Rerun policy.** Decide whether timing outputs are frozen with the quoted numbers (current state) or
   the prose should be rewritten to quote ranges so reruns stay consistent.
5. **Analogy proposals** above, especially the two outbox trays.
6. **Prerequisites.** F1-36 and F1-37 list SP201 as "may run in parallel" (atomics are used in the labs
   and explained locally); confirm with the SP faculty or move the atomics material.
