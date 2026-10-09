# SP203 Concurrency, atomics and the memory model — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; brief `/home/claude/uni-prompts/SP203.txt`).
Level L2–L3, 4 credits. Faculty F2, analogy world: the restaurant building and its kitchen. Card "Maps to:
Curriculum 1.1 (memory model), B10 preview, reading items 3 and 28". Where the course card items are:

- Lab "Bounded producer/consumer queue with ThreadSanitizer clean; measure contention": F2-37
  (`bounded_queue.hpp`, `pc_test.cpp` = the curriculum B10 test "10 million items, no loss, no duplicates",
  run under ASan/UBSan and under ThreadSanitizer; `pc_bench.cc` measures throughput and wait counts).
  Contention is also measured in F2-36 (`contention.cc`) and F2-38 (`costs.cc`).
- Forensic "The deadlock at 3 a.m." (lock-order inversion from thread dumps): F2-40, with a real GDB thread
  dump of a really deadlocked process (`thread_dump.out`).
- Exams Q/M/F/P: Check-yourself questions in every chapter; P ("fix a racy program and prove it with
  tools") is the F2-35 lab and forensic lab (TSan before/after) and recurs in F2-36/F2-38/F2-39.
- Project "a thread pool with tests and a benchmark": F2-41 mini-project (course project), seeded by
  `thread_pool.hpp`, `pool_test.cpp` (6 tests, ASan/UBSan and TSan) and `pool_bench.cc`.
- Curriculum B10 acceptance tests quoted verbatim and passed in user space: producer/consumer (F2-37
  `pc_test`) and "The lock-order checker reports a deliberately inverted pair in a test build" (F2-40
  `lock_order_checker.hpp` + `checker_test.cpp`).

## Files

- Chapters `F2-34.html` … `F2-42.html`: all 21 template sections + Answers (forensic key as
  `<ID>-forensic-key`), Jargon box, Transition box, two inline SVG figures each, claim tags on every
  factual paragraph, real runs via `data-run`. Titles match the catalogue in guide section 7
  ("The C++ memory model and memory orders", "Deadlock and lock ordering", "Thread pools and tasks",
  "Lock-free basics").
- `glossary.json`: 74 four-part entries generated from the Jargon boxes. 18 terms that other courses
  already define (Thread, Data race, Interleaving, Ring buffer, Atomic operation, Read-modify-write (RMW),
  False sharing, Acquire and release, Sequential consistency, Memory fence, AddressSanitizer, Amdahl's law,
  Cache coherence, Invariant, Memory consistency model, Multicore processor, Store buffer, Undefined
  behaviour) are copied verbatim from those courses' files with SP203 chapter ids, so `build.py` merges
  the chapter lists and keeps the original text. Every `#gl-…` link in the nine chapters resolves.
- Labs `university/labs/F2-34` … `F2-42`, each with `run.sh`. Scratch scripts (validator, glossary
  generator) are in `/tmp/claude-0/work/SP203/`, not in the repository.

## Listings run

Every lab passed `university/labs/run_lab.sh university/labs/<ID>` from the repository root (final run
of all nine on 2026-10-09). Toolchain `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`; listings `.cpp`
with the course flags `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`;
`.cc` files are built only by `run.sh` (ThreadSanitizer builds with `-O1 -g -fsanitize=thread`, timing
programs with `-O2`, racy programs that must not run under the course build). Also used:
GDB 15.1, `aarch64-linux-gnu-g++` 13.3.0, qemu-aarch64 8.2.2. Machine: Linux x86_64 KVM guest, 4 vCPUs,
**shared with many other build agents during the whole run**.

| Chapter | Runs (log names) | Result |
|---|---|---|
| F2-34 | `start_join`, `arguments`, `jthread_stop`, `worked`, `not_joined`, `dangling`, `speedup` | pass; `not_joined` exit 134 (std::terminate, intended), `dangling` exit 1 (ASan stack-use-after-return, the forensic evidence); `speedup` measured |
| F2-35 | `interleavings`, `lost_update_plain` (5 runs), `lost_update_tsan` (66), `lost_update_fixed_tsan`, `hoisted_flag_O0`, `hoisted_flag_O2` (timeout 124, intended), `hoisted_flag_asm`, `stats_tsan` (66), `stats_plain` (5 runs) | pass; 66 = ThreadSanitizer report, expected |
| F2-36 | `bank`, `bank_tsan`, `guard_exception`, `menu_shared`, `menu_shared_tsan`, `worked`, `forgot_unlock`, `contention` | pass, exit 0; `contention` measured |
| F2-37 | `lost_wakeup`, `pc_test` (10 M items, PASS; `pc_test.timeout` = 120 s), `pc_test_tsan` (PASS, clean, 21–32 s), `worked`, `if_not_while`, `pc_bench` | pass, exit 0; `pc_bench` measured |
| F2-38 | `atomic_counter`(+`_tsan`, `_asm`), `cas`(+`_tsan`), `lock_free_query`, `spinlock`(+`_tsan`), `needs_libatomic` (**expected-fail**: link error without `-latomic`), `needs_libatomic_linked`, `oversell_plain` (5 runs), `oversell_tsan` (no warning, by design), `costs` | pass; `costs` measured |
| F2-39 | `message_passing`, `mp_release_tsan`, `mp_relaxed_tsan` (66), `relay`(+`_tsan`), `relaxed_counter`(+`_tsan`), `orders_x86`, `orders_arm64` (compiled only), `publish_config_tsan` (66), `publish_config_arm64` (compiled only), `publish_config_qemu` | pass; **`publish_config_qemu` is untested on hardware** (QEMU user mode on x86-64; its log carries a `hardware:` line saying so) |
| F2-40 | `ordered_transfer`(+`_tsan`), `checker_test` (PASS), `checker_test_tsan` (66 expected: TSan also reports the deliberate inversion), `inversion_tsan` (66), `waitfor`, `ledger` (137: killed after the dump, intended), `thread_dump` (GDB attach) | pass |
| F2-41 | `futures`(+`_tsan`), `pool_test`(+`_tsan`, ALL TESTS PASSED), `worked`, `starvation`, `pool_bench` | pass, exit 0; `pool_bench` measured |
| F2-42 | `spsc_test`(+`_tsan`), `treiber_stack`(+`_tsan`), `worked`, `aba_replay`, `ring_bench` | pass, exit 0; `ring_bench` measured |

**Rerun-sensitive outputs.** Timing outputs (`speedup`, `contention`, `pc_bench`, `costs`, `pool_bench`,
`ring_bench`) and the outcomes of racy programs (`lost_update_plain`, `stats_plain`, `oversell_plain`,
`oversell_tsan`, which waiter starves in `if_not_while`) change on every run. They changed a lot between
the runs of this build because of machine load (for example `pool_bench`'s large-task row went from 2.5×
faster than serial to no faster). The prose was rewritten after the final run so that it states only what
held in every run of this build, and says explicitly where results varied; thread-dump addresses are
referred to by GDB value numbers (`$3`, `$8`), not by address. If a later rerun breaks one of these
"held in every run" statements, the prose must be adjusted, not the output. The racy forensic programs
in F2-35 and F2-38 now run five times each so that at least one run normally shows the symptom; the
ThreadSanitizer report is the deterministic evidence.

## Unverified and untested-on-hardware boxes (20)

- F2-34: forward-progress wording in the standard; whether the 4 vCPUs are dedicated cores.
- F2-35: list of forbidden compiler transformations (invented loads/stores); TSan facts (cannot combine
  with ASan, exit code 66, slowdown); TSan start-up failure workaround (not reproduced).
- F2-36: `shared_mutex`/pthread fairness; glibc mutex fast path and futex protocol.
- F2-37: spurious wake-ups permitted by the standard and notify-under-lock advice; glibc condvar algorithm.
- F2-38: the false-sharing measurement is not clean on this machine (2-thread rows equal, padded counters
  slow down with thread count); LOCK cache-locking vs bus-locking, implicit lock of `xchg`, Arm atomics.
- F2-39: release sequences and `memory_order_consume`; TSO and AArch64 ordering rules;
  **untested on hardware**: the forensic program's misbehaviour on real Arm (no Arm machine; QEMU user
  mode proves nothing about ordering).
- F2-40: lockdep and `hierarchical_mutex` descriptions, `std::lock` algorithm; glibc lock-word value 2
  and 0 % CPU of a deadlocked process (not measured).
- F2-41: `broken_promise` on unrun `packaged_task`, blocking `std::async` future destructor, pool-sizing
  rule of thumb; thread-creation cost breakdown and future shared-state allocation.
- F2-42: progress-guarantee definitions, Treiber/Michael attributions, `atomic<shared_ptr>` lock in
  libstdc++, model checker names; `cmpxchg16b`/Armv8.1 CAS/exclusive monitors (no AArch64 build here).

All book/document sources (D-tags) are "Title only — not opened during this build" (dossier gate G1 open):
ISO C++ draft, Williams (item 3), McKenney (item 28), Nagarajan/Sorin/Hill/Wood (item 90), TSan docs and
paper, C++ Core Guidelines, OSTEP (item 2), Intel SDM, Arm ARM, futex(2)/Drepper, Boehm–Adve, Coffman et
al., GDB manual, kernel lockdep doc, Herlihy–Shavit, Treiber report, Michael (hazard pointers). Only S1
(Systems Curriculum) was opened. Source numbering is per chapter but kept consistent across the course
(D1 standard, D2 Williams, D3 McKenney, D4 Primer, D5 TSan, D6 Core Guidelines, D7 OSTEP, D8 Intel SDM,
D9 futex, D10 Arm ARM, D11 Boehm–Adve, D12 Coffman, D13 GDB, D14 lockdep, D15 Herlihy–Shavit,
D16 Treiber, D17 Michael).

## Analogy proposals (not in the guide 8.1 registry — need the Analogy Keeper's approval)

Registered mappings used as given: threads = cooks in one kitchen (F2-34), data race = two cooks writing
on one order slip (F2-35), mutex = single key to the spice cabinet (F2-36), atomic = tally counter
(F2-38); their registered limits are quoted in the "Where the analogy breaks" sections.

| Concept | Proposed mapping | Breaks where | Chapter |
|---|---|---|---|
| Condition variable | the bell at the pass | a ring nobody hears is lost; bells can ring by themselves (spurious wake-ups); hearing it does not get you there first | F2-37 |
| Release / acquire | put the plate down before raising the flag; look at the plate only after seeing the flag | machines reorder from many sources (compiler, CPU, store buffer), not one "assistant" | F2-39 |
| Deadlock | two cooks, each holding one key and waiting for the other's | real cycles can be long and involve joins/queues; programs never notice or negotiate | F2-40 |
| Thread pool / future | a fixed team of cooks taking tickets from the order rail / the guest's numbered receipt | a receipt (future) can be read once; cooks are not limited by stoves (threads by cores) | F2-41 |
| Lock-free (SPSC) | chef and waiter each move only their own chalk mark on the pass | counters are not seen instantly; CAS cannot see ABA; memory can vanish | F2-42 |

## Decisions for the owner

1. **Noisy shared machine.** All timing results were measured while many other agents were building on
   the same 4 vCPUs. Several measurements (F2-34 speedup, F2-38 false sharing, F2-41 large tasks) did not
   show the textbook effect reliably. The chapters say so instead of quoting the expected numbers. Decide
   whether to re-run the timing programs on a quiet or dedicated machine before publication.
2. **Arm memory ordering untested on hardware (F2-39).** Only compiler output and a QEMU user-mode run
   exist. An Arm board (Raspberry Pi 4/5) run of `publish_config.cc` in a loop, and of a store-buffering
   litmus test, would turn the untested box into evidence.
3. **libatomic.** The course build command cannot link `atomic<T>::is_lock_free()` for 16-byte types; F2-38
   keeps that as an expected-fail listing and runs a second build with `-latomic`. Decide whether the
   course command should add `-latomic` (it would hide this lesson).
4. **Benchmark sizes reduced for build time.** `pc_bench.cc` moves 400,000 items per configuration
   (2 million took over five minutes under load); `pc_test` keeps the curriculum's 10 million items and
   needs `pc_test.timeout` = 120 s. TSan run of `pc_test` takes 21–32 s here.
5. **Cross-lab include.** `F2-42/ring_bench.cc` includes `../F2-37/bounded_queue.hpp` to compare with the
   same queue. If labs must be self-contained, copy the header into F2-42.
6. **Forensic programs are reproductions.** Scenarios (the 3 a.m. ledger, the oven board, the theatre)
   are illustrative; each key ends with "How the evidence was made". The F2-40 dump is a real GDB attach to
   a really deadlocked process, trimmed only as its log says.
7. **Glossary text for shared terms.** Where SP203's Jargon box words a shared term differently (e.g.
   Ring buffer, Read-modify-write), the published glossary keeps the earlier course's text (build order).
   The Glossary Keeper may want one merged wording.
8. **Curriculum seed "TLS (thread-local storage)"** is used (F2-40's checker uses `thread_local`) but has
   no four-part entry yet; F2-34/F2-40 could take it in a later pass.
