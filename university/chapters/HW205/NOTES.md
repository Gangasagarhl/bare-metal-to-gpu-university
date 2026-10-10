# HW205 Storage and network hardware — author notes

Author / Lab Engineer run, build of 2026-10-09 (no internet; brief `/tmp/claude-0/prompts/HW205.txt`).
Level L2–L3, 3 credits. Faculty F1, analogy world: the restaurant building and its kitchen; storage is
"the warehouse across town". Course card items and where they live:

- Lab "Inspect NVMe and NIC devices in QEMU": F1-50 (`qemu_pci`, `nvme_trace`, `trace_summary`) and
  F1-52 (`qemu_nics`, `ring_trace`), all on QEMU 8.2.2 device models (TCG, no KVM).
- Lab "Wireshark capture of your machine's traffic": F1-51. tshark 4.2.2 captured the build machine's
  own loopback traffic (`loopback`) and an emulated e1000's traffic (`qemu_capture`); a capture of a
  physical NIC is a student step marked "untested in this build" (the container has no physical
  network the author may capture).
- Forensic "Link up, no packets" (RGMII delay mismatch): F1-51's forensic lab, evidence from the
  course's own toy MAC–PHY model (`rgmii_evidence`, answer key `rgmii_sweep`).
- Exam P "explain an NVMe submission/completion trace": F1-50, built on a real QEMU `pci_nvme_*` trace
  of SeaBIOS reading LBA 0 (`nvme_trace`) and the summariser `nvme_trace.cc` (`trace_summary`).
- Project "annotated diagram set of the path of one network packet and one disk block": the disk-block
  path is the mini-project of F1-49 (part 1) and F1-50 (part 2); the packet path is the mini-project of
  F1-51 (part 1), F1-52 (part 2) and F1-53 (part 3, RDMA lane). F1-54's mini-project is a memory-error
  monitor.

## Files

- Chapters: `F1-49.html` … `F1-54.html`. Each has all 21 template sections plus Answers (forensic key
  as `<ID>-forensic-key` inside Answers), the Jargon box, the Transition box, two inline SVG figures,
  claim tags on every factual sentence that needs one, line-by-line tables for every listing, and
  unverified and untested-on-hardware (or equivalent warning) boxes. Validated with the course's own
  checker (html.parser balance, id prefixes, no URLs, no `<script>`, h2 order, every `src` tag defined
  and every source used, every `data-src`/`data-run` file present).
- Prose length (rough word count of the whole fragment, including tables and figure text): F1-49 ≈ 6,200,
  F1-50 ≈ 6,200, F1-51 ≈ 6,000, F1-52 ≈ 5,800, F1-53 ≈ 5,200, F1-54 ≈ 5,600.
- Glossary: `glossary.json`, 55 four-part entries generated from the Jargon boxes (term strings are the
  `<dt>` texts, so every `#gl-…` link of the six chapters resolves; no term collides with another
  course's glossary). Sources in the entries point to the chapter's D/R tags and say "pending
  verification" where a document tag is involved.
- Labs: `university/labs/F1-49` … `F1-54`, each with `run.sh`. Binaries and capture files are deleted by
  the scripts; only sources, `.out` and `.log` files remain.

## Listings run

Every lab passed `university/labs/run_lab.sh university/labs/<ID>` (status 0) from the repository root,
last run on 2026-10-09 after the final edits. Toolchain `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`,
`-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`; QEMU 8.2.2 (SeaBIOS
1.16.3-debian-1.16.3-2, iPXE ROMs from the distribution); TShark 4.2.2; nasm 2.16.01; Python 3.

| Chapter | Run (log name) | Result |
|---|---|---|
| F1-49 | `ftl`, `lba`, `forensic` (toy FTL / image file) | pass (exit 0) |
| F1-49 | `my_disks` (lsblk, lspci of the build VM) | pass (exit 0); virtual disks only |
| F1-50 | `nvme_ring`, `nvme_forensic` (toy queue model) | pass (exit 0) |
| F1-50 | `qemu_pci` (monitor `info pci`, qtree, pci.ids) | pass (exit 0); emulated controller |
| F1-50 | `nvme_trace` (SeaBIOS boots from QEMU NVMe; boot sector exits via isa-debug-exit) | expected exit 33 |
| F1-50 | `trace_summary` (`nvme_trace.cc` on the real trace) | pass (exit 0) |
| F1-51 | `frame`, `rgmii_evidence`, `rgmii_sweep` | pass (exit 0); RGMII parts are a toy model |
| F1-51 | `frame_decode` (tshark FCS check), `crc_check` (Python zlib) | pass (exit 0) |
| F1-51 | `qemu_capture` (iPXE DHCP on QEMU e1000, `filter-dump`) | expected exit 124 (20 s time limit) |
| F1-51 | `loopback` (tshark on `lo`) | pass (exit 0) |
| F1-52 | `ring`, `rx_stall` (toy ring) | pass (exit 0) |
| F1-52 | `qemu_nics` (monitor `info pci` for e1000, e1000e, virtio-net) | pass (exit 0); emulated |
| F1-52 | `ring_trace` (e1000e trace events + tshark of the same run) | expected exit 124 (20 s time limit) |
| F1-53 | `rdma`, `stale_key` (toy RDMA model) | pass (exit 0) |
| F1-53 | `rdma_check` | exit 0; **untested on hardware** (no `/sys/class/infiniband`, no ibv tools) |
| F1-54 | `secded`, `thermal`, `ecc_log` | pass (exit 0); thermal and ECC log are toy models |
| F1-54 | `sensors` | exit 0; **untested on hardware** (thermal folder empty, no hwmon, no EDAC) |

Untested on hardware overall: no physical disk, SSD, NVMe controller, NIC, PHY, RDMA NIC, thermal
sensor or ECC memory was used. Every chapter says so in a warning box (F1-49 got one at the end of
this run). Real-device observations are limited to the build VM's self-description (virtio disks,
ROTA 1, 512/4096-byte sectors) and its loopback traffic.

Notable real evidence: the QEMU NVMe trace (2,887 lines; SQ0 wraps 63→0, CQ0 wraps 255→0; 257
Identify, 1 Create CQ, 1 Create SQ, 1 Read; 260 completions, all status 0); the e1000e trace whose
transmit-descriptor lengths (429, 70, 429, 441, 42) equal tshark's frame lengths in the same run;
tshark's independent FCS verdict ("should be 0x83fc8600") matching Listing 1's CRC of the damaged frame.

Fixes made during the build (each run again afterwards): FTL garbage collection made lazy (sequential
WA went from 1.90 to 1.00; the chapter says the earlier log was not kept); toy NIC start-up attached a
buffer to every slot; SECDED decoder crashed with `std::out_of_range` for syndromes ≥ 72 (now
"detected"; F1-54 mentions it under Common mistakes); ECC log shortened, sorted, summary removed;
RDMA model's always-zero counter removed; RGMII sweep annotation corrected; tshark preference
`eth.assume_fcs` (obsolete in 4.2) replaced by `eth.fcs:Always`; monitor runs without `-S` so BARs are
assigned.

## Claims that could not be verified (sources title only, gate G1 open)

All D-sources are "title only — not opened during this build". Unverified boxes, by chapter:

- **F1-49**: names of the TRIM-type commands (ATA DATA SET MANAGEMENT/TRIM, NVMe Dataset Management
  deallocate, SCSI UNMAP). Untested-on-hardware box added (toy FTL, no real drive measured).
- **F1-50**: NVMe register names and offsets (CAP, VS, CC, CSTS, AQA, …), entry sizes 64/16 bytes,
  the doorbell stride formula; untested-on-hardware box (QEMU controller only).
- **F1-51**: 64-byte minimum frame, 1500-byte payload, preamble/SFD lengths, clause-22 register
  numbers, RGMII delay size; capture-point rule for padding/FCS on physical NICs; Linux `phy-mode`
  devicetree values (rgmii, rgmii-id, rgmii-rxid, rgmii-txid). The toy PHY register map is the
  course's own and says so.
- **F1-52**: e1000e register names beyond RDT (RDBAL/RDBAH/RDLEN/RDH, TDBAL…TDT, RAL0/RAH0, ICR) and
  descriptor bit meanings; the virtio column of the comparison table; Linux DMA API and barrier names,
  NAPI. QEMU itself only names RDT.
- **F1-53**: real "receiver not ready" handling; every verbs API name (ibv_reg_mr, ibv_post_send,
  IBV_WC_REM_ACCESS_ERR, …); RoCE v2 UDP port 4791, PFC/ECN/DCQCN, `rdma_rxe`; untested-on-hardware box.
- **F1-54**: meanings of ACPI _TMP/_PSV/_CRT/_HOT/_TZP (the catalog gives only the names); ECC DIMM
  widths (DDR4 72-bit, DDR5 subchannels, on-die ECC, chipkill, scrubbing); #MC vector 18, MCA register
  names, CMCI, Linux EDAC sysfs layout, rasdaemon; page-offlining names; untested-on-hardware box.

Hardware numbers: none from memory appear outside these boxes. Arithmetic examples use made-up
values labelled as such (6,000 rpm disk in F1-49, toy units in F1-54).

## Analogy mapping proposals (for the F1 analogy registry)

Registered mappings used: storage = the warehouse across town ("SSDs have internal processors and
queues"); interrupt = doorbell (device rings the CPU). Proposed new mappings (owner to accept or
change):

| Concept | Proposed mapping | Chapter |
|---|---|---|
| HDD | the old warehouse hall with a turntable and a sliding ladder (seek = slide, rotational latency = wait for the turntable) | F1-49 |
| Flash page / block | lockers in drawers: a full locker cannot be refilled; whole drawers are emptied | F1-49 |
| FTL | Amira's ledger mapping box numbers to lockers, with nightly tidying (GC) | F1-49 |
| Write amplification / wear levelling / TRIM | boxes moved ÷ boxes delivered / using every drawer in turn / phoning to say "throw these away" | F1-49 |
| NVMe SQ / CQ | the circular order board / the circular done board in the office | F1-50 |
| NVMe doorbell | the bell wired to the warehouse counter with a number display (host → device) | F1-50 |
| Phase tag | the clerk's ink colour, changed each lap of the board | F1-50 |
| MAC / PHY | Rosa in the dispatch office (packs, labels, seals) / Kwame at the loading dock (road rhythm) | F1-51 |
| FCS | the tamper sticker whose pattern depends on the contents | F1-51 |
| MII-family interface / MDIO | the handover hatch / the office's telephone to the dock | F1-51 |
| RGMII delay mismatch | both sides "waiting a moment" before the handover | F1-51 |
| NIC receive ring / tail register | Lena's round rack of shelves with crates / the red marker | F1-52 |
| RDMA / registered region / rkey | the partner's driver using a service door / a sealed cold-room shelf / the card number | F1-53 |
| Thermal throttling (DVFS) | head chef Ana slowing the line on a hot evening (burners to a simmer) | F1-54 |
| ECC / syndrome / CE log | Tomás's ledger check digits / the line-and-column pointer / his log of fixed digits | F1-54 |

**Conflict to resolve:** the faculty's registered "doorbell" means an interrupt (device → CPU), but
NVMe's and RDMA's *doorbell registers* go the other way (host → device). F1-50 uses "the bell wired
to the warehouse counter" and its jargon entry warns about the clash; F1-52 calls the NIC interrupt
"the kitchen bell" and the tail write "the red marker", and its "Where the analogy breaks" list
explains the difference. The owner may prefer a distinct image for host→device doorbells (for
example "the order slip dropped into the warehouse's slot").

Character names are new per chapter (Amira and Leila F1-49, Mei F1-50 forensic, Rosa/Kwame/Jonas F1-51,
Lena/Amara F1-52, Rafael F1-53, Ana/Tomás/Ines F1-54); none is a real person.

## Decisions for the owner

1. **Doorbell analogy conflict** (above): accept the per-chapter wording or register a separate image.
2. **Wireshark lab**: accepted as loopback + QEMU capture + a student step on their own machine. If a
   capture of a physical NIC by the author is required, a machine with a network the author may
   capture is needed.
3. **F1-53 depth**: written as a preview with a toy model and an honest "untested on hardware" record;
   real verbs runs (Soft-RoCE or RDMA NICs) are left to DS401. Confirm, or provide a machine where
   Soft-RoCE may be loaded.
4. **F1-54 scope**: the chapter covers power (V²f, DVFS, P/C/S-states as a map), thermal throttling and
   ECC/MCA. Sensor and EDAC reading is a student step; the build VM exposes none. Confirm that the
   physics of heat flow stays at the one-line model level (the HW1xx F1-03 chapter covers power and heat).
5. **Toy models as forensic evidence**: all four network/memory forensic packs (RGMII, rx stall, stale
   key, ECC log) come from the course's simulators and say so. The NVMe and RGMII register maps in
   them are invented and labelled; check this is acceptable for "PHY register dump description".
6. **Word counts** exceed the 3,000 (L2) / 4,000 (L3) targets once tables and figure text are counted;
   prose alone is close to target. Trim if the owner wants shorter chapters.
7. **`.cc` listings**: `nvme_trace.cc` (F1-50) uses the `.cc` extension so that `run_lab.sh` does not
   compile it with the default flags; `build.py`'s `LISTING_EXT` does not list `.cc` or `.asm`, but the
   listings rendered in this build (checked in UNIVERSITY.html). Confirm that convention.

`python3 university/build/build.py` exited 0; none of its PROBLEM lines (320 "broken link" lines at the last run, all
`#gl-…` targets of other courses) mentions F1-49 … F1-54 or an HW205 glossary link.

## Owner rulings applied

Verification pass of 2026-10-10 (rulings in `university/OWNER_RULINGS.md`).

1. **Doorbell analogy conflict** (NVMe/RDMA doorbell registers ring host→device, the faculty's "doorbell" means an
   interrupt) → ruling **A3**: all mappings proposed above are approved and each course keeps its own cast; no
   separate image is registered. **Decided by verifier:** keep the per-chapter wording (F1-50's bell "wired to the
   warehouse counter", F1-52's "red marker" for the tail write and "kitchen bell" for the interrupt); the jargon entry
   of F1-50 and the "Where the analogy breaks" lists of F1-50 and F1-52 already state the clash. The sentence
   "(Owner decision recorded in NOTES.md)" in F1-52 was replaced by "(ruling A3: the two images stay distinct)".
2. **Wireshark lab** (loopback + QEMU capture + a student step on the learner's own machine) → ruling **C3**: accepted.
   The student step stays marked "untested in this build" and says what is needed (a computer whose network the
   learner may capture, Wireshark or tshark, administrator rights).
3. **F1-53 depth** (preview with a toy model, real verbs runs left to DS401) → rulings **C3** and **D1**: accepted;
   the chapter now names the reference kit for the real runs (KIT.md: two NVIDIA ConnectX-6 Lx NICs cabled back to
   back, or Soft-RoCE on ordinary Ethernet) in its "Untested on hardware" box.
4. **F1-54 scope** (one-line heat model; sensors and EDAC as a student step) → no ruling covers it; **decided by
   verifier:** keep the one-line model; HW101 (F1-03) carries the physics of power and heat. The sensor/EDAC step
   stays "untested on hardware" (ruling C3) and says what is needed (a physical Linux computer with hwmon/thermal
   sysfs entries and, for EDAC, ECC memory with an EDAC driver).
5. **Toy models as forensic evidence** (RGMII, rx stall, stale key, ECC log; invented NVMe and PHY register maps)
   → ruling **A2**: approved. Every forensic pack now says in one sentence that it is the course's own model and names
   the real thing it stands for (the FTL firmware of an SSD; QEMU's NVMe device and the NVM Express Base
   Specification's queue rules; a MAC driver's counters and the PHY's IEEE 802.3 clause 22 registers read over MDIO;
   the e1000e descriptor ring traced in the lab; libibverbs (rdma-core); the Linux EDAC/rasdaemon memory-error
   reports).
6. **Word counts** above the L2/L3 targets → ruling **A1**: accepted as written; nothing trimmed for length.
7. **`.cc` listing** (`nvme_trace.cc`, compiled by `run.sh`, not by `run_lab.sh`) → no ruling covers it; **decided by
   verifier:** keep. `build.py` inserts any `data-src` path (checked in the build of 2026-10-10: the listing renders),
   and the lab-files appendix lists every file of the folder regardless of extension.
- Ruling **A5** applied to the lab re-runs (see below). Rulings **A8** (committed keys), **A10** (licence placeholder)
  and **D4** (e-stop) do not apply: HW205's lab folders contain no keys, no `LicenseRef-Uni-Lab` marker and no robot
  image. Ruling **B4** (exams): written by the Exam Writer pass, not here. Ruling **A7** (glossary duplicates): the
  verifier's cross-course check found collisions that the build notes had missed. "Descriptor" (F1-52) collided with
  HW204's USB "Descriptor" (a different meaning) and was renamed "Descriptor (NIC ring)" in `glossary.json` and in the
  chapter's jargon list and glossary links. "Descriptor ring" duplicates HW204 F1-44 with the same meaning: HW204 is the
  earlier course, so its wording is canonical and the HW205 entry's source now says so. HW205's NVMe/virtio terms also
  appear in DR301 (11 terms) and "Block device" in OS304; HW205 comes first in the build order, so HW205's wording is
  canonical for those and nothing was changed here. "DMA (direct memory access)" and "Interrupt (from a NIC)" keep
  their qualified names because HW204 defines the general terms. Ruling **C4**:
  status of all six chapters is "internally checked · hardware steps untested".

## Verification pass

Fact-Checker / Source Researcher agent, 2026-10-10. Dossiers: `university/_dossiers/F1-49…F1-54.dossier.html`;
QA records: `university/qa/F1-49…F1-54.json` (all "factcheck": "done"; status "internally checked · hardware steps
untested", ruling C4).

**Opened (web, 2026-10-10):** OSTEP v1.10 chapters 36 "I/O Devices", 37 "Hard Disk Drives" and 44 "Flash-based SSDs"
(F1-49); QEMU documentation "NVMe Emulation" and QEMU v8.2.2 `include/block/nvme.h` on the GitHub mirror (F1-50);
Linux kernel documentation "PHY Abstraction Layer" (F1-51); QEMU v8.2.2 `hw/net/e1000_regs.h` (the ten ring register names, F1-52);
Intel 8254x Software Developer's Manual Rev 4.0 — title page and introduction only, the fetched text was truncated
before the register chapter (F1-52).
**Not opened:** the NVM Express Base, PCIe Transport and NVM Command Set specifications; IEEE 802.3 and the RGMII
specification; the Intel 82574 datasheet and QEMU's `e1000x_regs.h` (interrupt, address and descriptor bits); the VIRTIO specification; LDD3; the kernel
DMA-API, memory-barriers, NAPI, EDAC, machine-check and hwpoison documents; the InfiniBand Architecture
Specification (registration-walled) and the rdma-core manual pages; the NVIDIA GPUDirect and RDMA programming
manuals; the IANA port registry; the ACPI 6.5 thermal chapter; the Intel SDM and AMD APM; JEDEC JESD79-4/-5
(registration-walled); the Patterson/Hennessy and Harris textbooks (not freely readable). Reason: the web-fetch
budget (400 per hour, shared by every agent) was exhausted for most of this pass; each attempt after the first few
was refused. Every claim that only those documents can settle sits in an unverified box that names the document and
section to check; the sources lists say "not opened" honestly (AH-4).

**Corrections made:** F1-49 retagged to OSTEP sections, cell wording and SLC/MLC/TLC endurance corrected, read-modify-
write marked as reasoning, bad-block/ECC sentences softened, hardware paragraph rewritten around OSTEP ch. 36. F1-50
register names, CC fields, entry sizes, opcodes and CNS values confirmed from QEMU's header (tier 4, "what this
implementation does"); sources D1–D5 say what was and was not opened; D6 added. F1-51 RGMII delay (1.5–2 ns, added by
PHY, MAC or traces) and the four `PHY_INTERFACE_MODE_RGMII*` modes confirmed from the kernel document; D6 added.
F1-52 ring register names RDBAL…RDT/TDBAL…TDT confirmed from QEMU's header (D7 added); "Descriptor" renamed
(ruling A7); Figure 1 ownership now also in text. F1-53 Figure 1 legend no longer names
colours. All six forensic packs name the toy model as the course's own and say what it stands for (ruling A2); all
"untested on hardware" boxes say what would be needed (rulings C3, D1; `build/KIT.md`).

**Labs:** all six folders re-run with `run_lab.sh` and each `run.sh` (expected exit codes 33 for `nvme_trace`, 124 for
`qemu_capture` and `ring_trace`); every listing passed; differences were only dates, timestamps, ephemeral ports and
one virtual disk's size; recorded files restored with `git checkout -- university/labs/F1-49 … F1-54` (ruling A5).
Untested on hardware: `rdma_check`, `sensors`, and the physical-NIC and real-SSD student steps.

**Build:** fragments validated (balanced, ids prefixed, no URLs, no scripts); `build.py` run and
`university/UNIVERSITY.html` restored afterwards.
