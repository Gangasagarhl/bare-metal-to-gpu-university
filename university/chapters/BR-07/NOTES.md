# BR-07 — From a PC to an SoC board: author notes

Bridge chapter, level L4, placed before F4-31 (prerequisite of DR403), referenced by RB403
(F9-65, F9-67) and DN302 (F10-26). Lab: `university/labs/BR-07`, run with
`university/labs/run_lab.sh university/labs/BR-07` from the repository root: status 0
(about 40 s; last full run on 2026-10-10 after the last change).

## What the chapter does

- Carries over (in depth, with evidence): drivers written against specifications; DR401's universal
  method mapped step by step to a board (table in Layer 3); DR402's devicetree reader
  (`F4-23/fdt.h`, `F4-23/fdt_tool.cc`) and `F4-27/make_dtb.py` reused **unchanged** by relative path.
- Changes (in depth): description instead of enumeration (tree vs QEMU memory map of the board
  model), boot ROM and chain (ROM words and first instructions of `sifive_u`, OpenSBI hand-over),
  explicit clocks/resets/pins/regulators/power domains (dependency inventory on four trees),
  non-coherent DMA (cache model), documentation quality (curriculum 11.4, F4-15 tiers).
- Traps, each with a real run: (1) clock gate "reads zeros" — host model `clock_gate.cpp`;
  (2) bus-address translation — bare-metal AArch64 program on QEMU `raspi3b` that prints through
  the translated address then stores to the bus address (synchronous external abort, ESR 0x96000050);
  (3) on-board storage without recovery — QEMU Arm `virt` boot flash (EDK2) overwritten with
  another board's firmware: silence; restored from a verified backup: banner again.
- Lab = the card's lab on the board model `sifive_u` (known-good OpenSBI boot, dumpdtb, F4-23
  checklist, memory map, dependency inventory, bring-up plan template `bringup_plan.txt`).
- Forensic lab: a kernel with virt's hard-coded console address (0x10000000) is silent on the
  board model because that address is the PRCI clock controller (QEMU guest_errors evidence).
- Figures: Fig. 1 before/after (PC vs SoC board); Fig. 2 providers-before-consumers graph of the
  teaching board (from R7).

## Listings run (all in this build; no real hardware)

| Run (`.log`) | Exit | Status |
|---|---|---|
| clock_gate | 0 | pass |
| dma_coherence | 0 | pass |
| known_good | 124 | pass (time limit expected; check = OpenSBI banner); untested on hardware |
| bootrom | 0 | pass; untested on hardware |
| board_dump | 0 | pass; untested on hardware |
| board_checklist | 0 | pass |
| board_mtree | 0 | pass; untested on hardware |
| deps_board | 0 | pass |
| deps_teach | 0 | pass |
| deps_virt | 0 | pass |
| deps_pi3 | 0 | pass |
| translate_build | 0 | pass |
| translate_demo | 3 | expected-fail (exception handler stopped the run: the trap); untested on hardware |
| translate_addr2line | 0 | pass |
| flash_ok | 124 | pass (time limit expected; check = EDK2 first line); untested on hardware |
| flash_bad | 124 | expected-fail (no output at all: the bricked state); untested on hardware |
| flash_bad_trace | 0 | pass (evidence log); untested on hardware |
| flash_recover | 124 | pass (banner restored); untested on hardware |
| forensic_build | 0 | pass |
| forensic_virt | 124 | pass (greeting printed); untested on hardware |
| forensic_board | 124 | expected-fail (silent kernel: forensic evidence); untested on hardware |
| forensic_errors | 0 | pass (evidence log); untested on hardware |

`run.sh` checks every expected outcome itself and fails the lab if one changes.
Volatile values (boot hart number in `known_good.out`, counts of repeated log lines in
`flash_bad_trace.out` and `forensic_errors.out`) are not quoted in the prose.

## Unverified boxes (6)

1. Layer 3 / chain: FU540 MSEL meaning, boot sources and formats (D4 not opened; only QEMU's model shown).
2. Layer 3 / clocks: PRCI clock indices, firmware-left-on clocks, gated-access behaviour and clock/reset order on real SoCs.
3. Layer 3 / DMA: default coherence convention per architecture (`dma-coherent` vs RISC-V `dma-noncoherent`), AArch64 `DC` and RISC-V Zicbom instructions; QEMU does not model incoherent DMA.
4. Code walk-through / trap 2: what a real Pi 3 does on a CPU store to 0x7e201000; BCM2835 bus-address scheme and ESR decoding from memory.
5. Trap 3: real-board recovery routes (FEL, mask-ROM mode, uuu, SD swap), flash protection; QEMU board can always be restored from the host.
6. Lab: board steps untested; Linux paths for the received devicetree (`/sys/firmware`, `/proc/device-tree`) and `dtc` usage from memory.

## Claims that could not be verified (all sources title only, gate G1 open)

D1 Devicetree Specification; D2 Linux kernel documentation (clock, reset, pinctrl, regulator,
power-domain bindings; DMA API); D3 Arm ARM (ESR classes, fault status, WnR, vectors, DC ops);
D4 SiFive FU540-C000 Manual; D5 QEMU System Emulation User's Guide; D6 OpenSBI docs and SBI spec;
D7 BCM2835 ARM Peripherals; D8 Patterson and Hennessy, Computer Organization and Design (edition to
record); D9 RISC-V Zicbom. The 3.3 V serial-adapter rule is quoted from curriculum 11.5 (C1), not
from a board document.

## Decisions for the owner

1. **Board stand-ins.** The card's lab says "on the board". With no board in the build, the lab
   uses QEMU `sifive_u` (a model of a real board, already D7's stand-in in F4-29) for the
   known-good boot and dump, QEMU `raspi3b` for trap 2 and QEMU Arm `virt` with its pflash boot
   flash for trap 3. Accept, or choose a reference board for the real-hardware version and
   record it in the dossier.
2. **Teaching board tree.** `teach_board.py` writes an *imaginary* board's DTB (vendor prefix
   `univ,`) because no QEMU machine's tree has resets, pin groups, supplies or power domains.
   It is labelled imaginary everywhere. Keep, or replace with a real board's tree once a source
   can be opened.
3. **Clock-gate model.** Trap 1 is a host model (our code), consistent with F4-35's `socsim`
   ("reads as zero"); no QEMU board models clock gating. Real behaviour stays in an unverified box.
4. **Length.** Prose is above the L4 lower target (the card asks for carries-over, changes and
   traps each in depth, plus a full lab, forensic lab and worked plan). Trim candidates if needed:
   the "universal method on a board" table and the extension steps.
5. **New glossary terms (6):** Known-good image; Dependency inventory (course term); Boot mode pins
   (boot source selection); Regulator (supply in a devicetree); Power domain; dma-coherent
   (property). Everything else links to existing DR402/DR403/HW/OS305 entries.
6. **Analogy.** Reuses F4-31's *proposed* mapping "devicetree = the building plan" (not yet in the
   registry) plus registered firmware = caretaker. New story elements (lights at the main panel =
   clock gate, architect's numbers = bus addresses, spare key = recovery path) are proposals for
   the F3/F4 registry: please accept or reject.
7. **Run cost.** The flash trap copies two 64 MiB firmware files into a scratch folder that
   `run.sh` deletes; nothing large stays in the repository.
