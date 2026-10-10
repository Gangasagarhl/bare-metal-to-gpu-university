# BR-06 — notes for the owner

## Files
- `BR-06.html` — the chapter (20 h2 sections, plus the jargon box and the Transition box; 3 inline SVG figures).
- `glossary.json` — 5 new terms: Boot contract, Page granule, Plain char signedness, Cache-maintenance instruction, Entry probe. All other terms link to existing glossaries (see "Glossary links").
- `university/labs/BR-06/` — the lab; `university/labs/run_lab.sh university/labs/BR-06` exits with status 0 (about 15 s).

## Runs (all real, QEMU 8.2.2 TCG / qemu-user, GCC 13.3, clang 18)
| Run | Result |
|---|---|
| probe_x86, probe_aarch64, probe_aarch64_el2, probe_riscv64, probe_riscv64_m | pass (one unchanged core, SHA-256 in each log) |
| checklist | pass (9 rows x 3 machines from monitor + probes) |
| neutral | pass: 8/8 on x86-64 native (ASan/UBSan), aarch64 and riscv64 under qemu-user |
| litmus | runs; counts vary per run. Native x86: SB relaxed seen in some runs, others zero. aarch64 emulated: SB seq_cst (0,0) seen — QEMU artefact |
| codegen, ordering_model, pagesize_trap, dma_cache_model | pass |
| apic_x86 | pass (APIC read works); apic_aarch64, apic_riscv64 | expected fault (data abort / load access fault), addr2line -> probe_core.cc:67 |
| x86ism | expected compile failure on aarch64 and rv64gc; builds with zihintpause (different instruction) |
| cachemaint | assembled only — untested on hardware |
| forensic_diskcheck | expected failure on s390x (big-endian); forensic_fixed passes on 4 CPUs |

## Unverified / untested-on-hardware boxes (6)
1. Privilege-model register meanings (CPL, CurrentEL, scause codes, misa layout, ESR EC 0x25) — D2–D4 not opened.
2. Boot contracts (Multiboot, arm64 Image header, OpenSBI hand-off, PSCI SYSTEM_OFF ID, SBI SRST ID) — D5–D8 not opened.
3. Memory ordering rules (TSO, stlr/ldar ordering, RISC-V fence semantics); our model is simplified.
4. Cache maintenance semantics (DC CVAC/IVAC/CIVAC, Zicbom); QEMU's DMA is coherent, so no stale-cache failure can be shown — untested on hardware.
5. Hardware ordering: no Arm/RISC-V board. The qemu-aarch64 SB seq_cst (0,0) result is explained as a TCG artefact on an x86 host (hypothesis, QEMU docs not opened); needs a run of litmus.cc on real AArch64.
6. Worked-example bit decodes (CR0, misa, ESR) — recalled from D2–D4; cross-checked against OpenSBI's ISA line and addr2line.

All D-sources are "title only — not opened; dossier gate G1 open". No URLs.

## Decisions for the owner
- **Analogy registry proposal:** "noticeboard where notes may be read out of the order they were pinned, unless stamped" = weak memory ordering; "read-earlier-notes-first stamp" = release/acquire barrier. Used in the hook, Layer 1 and the jargon box, marked as proposed. "Where the analogy breaks" lists its limits.
- Reused F4-23's Ms Okafor (visiting three schools before opening her second one).
- **Big-endian stand-in:** the forensic uses qemu-s390x as the big-endian CPU (no big-endian Arm/RISC-V user-mode target in the container). F4-30 covers big-endian more broadly.
- **Overlap with F4-23:** BR-06 fills the checklist from QEMU's monitor + entry probes; F4-23 does it from a devicetree parser. BR-06 deliberately does not parse the DTB (only its magic) and links to F4-23 for the method.
- **Length:** prose is above the ~5,000-word L4 target once line tables and the answers are counted (~11,000 words of text in total including tables, sources and answers). Trim candidates: the Listing 16 (run.sh) table and some of the worked example.
- Litmus counts change every run, so the chapter describes patterns ("in some runs") and never quotes numbers; a rebuild will show different counts.
- x86 probe uses 32-bit Multiboot (QEMU -kernel), not UEFI; UEFI+ACPI are described from OS301/F3-24 and the documents, not re-run here.
- Row 8 (SMP start-up method) of the checklist is answered "from documents" because the memory map cannot show it.
