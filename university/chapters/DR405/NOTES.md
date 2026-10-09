# DR405 — Toward GPU drivers (optional): author notes

Chapters F4-42 to F4-47. Levels L4 (F4-42 to F4-45) and L5 (F4-46, F4-47); 4 credits. Prerequisites: DR302 and HW301.

This course maps to Track G (milestones G1 to G4) and to curriculum section 16 (16.1 to 16.3, and milestones G5 and G6). The course forensic, "Tearing", is in F4-46. Exam P, "the path of a compute dispatch from a HIP call to the GPU", is covered by F4-42: Layer 3, Figure 2 and Check yourself 2, 3 and 8.

Labs are in `university/labs/F4-42` to `university/labs/F4-47`. Each one passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All six were run again, one after the other, at the end of this build on 2026-10-09, after the last code change. The table below comes from the `.log` files of that final sweep.

## Toolchain (as recorded in the logs)

- **Compiler:** g++ 13.3.0 with GNU ld. Host programs use `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
- **HIP:** hipcc from HIP 5.7.31921 (clang 17), target gfx90a. There is no GPU, so its programs were compiled and their GPU code was generated, but nothing ran on a GPU.
- **QEMU:** 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18), TCG only (no KVM), q35 machine, with SeaBIOS 1.16.3-2 VGA BIOS images.
- **Other tools:** strace 6.8; Python 3.13 (for `../F4-03/qmp_drive.py`).
- **Headers:** linux-libc-dev 6.8.0-146.146 (DRM, KFD, virtio-gpu and pci_regs UAPI headers); libhsa-runtime-dev 5.7.1-2build1 (`hsa/hsa.h`).
- **Firmware files:** nvidia-firmware-580-580.178.04 (two GSP images, listed only).
- **Lab kernel:** the DR301 32-bit Multiboot kernel, with paging off.

## Cross-course dependencies (used unchanged, not edited)

| Path | Owner | What it provides | Used by |
|---|---|---|---|
| `../F4-01/` | DR301 | `lablib.sh`, `kbase.*`, `boot.S`, `kernel.ld` | F4-43, F4-44, F4-46 |
| `../F4-02/` | DR301 | `pci.*` | F4-43, F4-44, F4-46 |
| `../F4-03/qmp_drive.py` | DR301 | Drives QEMU: waits for serial lines, sends HMP `screendump`/`sendkey` | F4-43, F4-46 |
| `../F4-05/` | DR301 | `virtio.h`, `virtio.cc` (virtio transport) | F4-43, F4-46 |
| `../F4-43/` | DR405 | The F4-43 kernel sources, rebuilt by F4-46 `record` | F4-46 |

**Owner decision:** if DR301 changes these files, F4-43, F4-44 and F4-46 must be re-run.

## Listings run

**Nothing was run on real hardware or on a GPU.**

- QEMU runs are TCG and untested on hardware.
- **pass:** the run gave the expected result.
- **expected-fail:** a failure that the lab requires.
- **forensic:** evidence produced by a deliberately broken build or a model.

| Chapter | Run (`.log`) | Exit / result | Status |
|---|---|---|---|
| F4-42 | uapi_map | 0 | pass |
| F4-42 | aql_queue | 0 | pass |
| F4-42 | aql_forensic | 0 | pass (forensic replay, model) |
| F4-42 | vecadd | 1 ("no ROCm-capable device is detected") | expected-fail; **untested on hardware** |
| F4-42 | strace | 1 (vecadd's exit under strace; `/dev/kfd` ENOENT) | expected-fail; **untested on hardware** |
| F4-42 | isa | 0 | pass (GPU code generated, not run) |
| F4-43 | vgpu_check | 0 (0 differences) | pass |
| F4-43 | build | 0 | pass |
| F4-43 | vgpu | 33 | pass; untested on hardware |
| F4-43 | screens | 0 (MATCH, MATCH) | pass |
| F4-43 | trace | 0 | pass |
| F4-43 | res800 | 33 (MATCH) | pass; untested on hardware |
| F4-43 | forensic | 33 (frame 2 MISMATCH, as designed) | pass (forensic); untested on hardware |
| F4-44 | romcheck | 0 | pass |
| F4-44 | build | 0 | pass |
| F4-44 | probe | 33 | pass; untested on hardware |
| F4-44 | ati | 33 | pass; untested on hardware |
| F4-44 | compare | 0 (SAME, SAME, DIFFERENT) | pass |
| F4-44 | forensic | 33 (ROM reads zeros, as designed) | pass (forensic); untested on hardware |
| F4-45 | uapi_survey | 0 | pass |
| F4-45 | target_score | 0 | pass |
| F4-45 | versions | 0 | pass |
| F4-46 | record | 33 ×5 (EDID blobs kept as `edid_*.hex`) | pass; untested on hardware |
| F4-46 | edid_test | 0 (5 PASS) | pass |
| F4-46 | flipsim | 0 | pass (model) |
| F4-46 | tearing_log | 0 | pass (forensic evidence, model) |
| F4-47 | ref_triangle | 0 | pass |
| F4-47 | study_check | 0 (1 of 2 drafts rejected, as designed) | pass |

**Untested on hardware**, and stated in each chapter:

- **G1:** the ROCm runtime's ioctl sequence on a GPU.
- **G2:**
  - The display-change event and its handler.
  - The cursor moved by a C8 USB mouse. The cursor commands were checked only through QEMU's trace.
- **G3:** a real AMD card, ATOM BIOS parsing, and the comparison with amdgpu output.
- **G4 and G6:** prototypes.
- **G5:** everything on Intel hardware (state dump, GMBUS, flips, mode set, comparison with Linux).

## Per chapter: unverified claims and boxes

### F4-42 What a GPU kernel driver does

Unverified boxes:

1. **Dispatch path, steps 2–7:**
   - the exact ioctls ROCm 5.7 uses;
   - "header last with a release store" (HSA specification, D8);
   - command-processor internals;
   - user SGPR meanings (LLVM AMDGPU user guide, D9).
2. **GEM, TTM, GART, eviction with queue preemption, the DRM scheduler and time-outs, latching at vblank, the GSP's role, WDDM component names.** These come from memory and D10.

Other notes:

- **Opened in this build:** UAPI headers (D6) and `hsa.h` (D7). The quoted comments are verbatim from these.
- **DRIVERS.html:** the names `dxgkrnl.sys`, `amdkmdap` and `amdkmdag` and their descriptions are quoted from DRIVERS.html, which was opened in this build.
- **Title-only sources:** D1–D5, D8, D9.

### F4-43 A virtio-gpu driver

Unverified boxes:

1. **Statements not checked against the OASIS virtio-gpu text (D1):**
   - resource id 0 is reserved;
   - the rule for the transfer offset;
   - command ordering;
   - the fence flag.
2. **No vblank event** in virtio-gpu 2D.

Warning box (untested): the G2 display-change event and the USB-mouse cursor.

Inference: the QEMU EDID values come from QEMU's generator (observed).

### F4-44 Probing a real GPU read-only

Unverified boxes:

1. **Expansion ROM / PCIR field offsets and the checksum rule.** These are from memory of the PCI Firmware Specification. They are consistent with four SeaBIOS images.
2. **ATOM BIOS layout:**
   - pointer at 0x48;
   - "ATOM" signature;
   - master command/data tables;
   - which amdgpu debug output lists them.
3. **Capabilities of a real discrete GPU, and resizable BAR.**
4. **Hardware paragraph:** shared decoders, the GOP in the UEFI image, amdgpu's VBIOS sources.

Inferences, marked as such in the text:

- QEMU patches the ati-vga ROM's device ID (file 5159, read 5046) and fixes its checksum.
- The shadow copy at 0xC0000 differs because the ROM code changes its own copy.

Neither was checked in the QEMU or SeaBIOS source.

Observation reported without comment: bochs-display's config class is 0x038000, but its ROM PCIR says 0x030000.

### F4-45 Open GPU documentation beyond AMD and NVIDIA

Unverified boxes:

1. **Every family statement from curriculum table 16.1**, including that the GSP images are signed.
2. **Panthor merged after Linux 6.8; asahi outside mainline** (from memory).
3. **The general description of firmware-fronted GPUs.**

Other notes:

- `target_score.in` holds the author's ratings, and the weights and thresholds are a teaching choice. Thresholds were set so that the verdicts reproduce 16.3, and the text says so.
- **Owner decision:** keep these ratings, or have the Source Researcher re-rate them from opened documents.

### F4-46 Native display on an Intel integrated GPU

Unverified boxes:

1. **EDID byte layout** (VESA E-EDID not opened). The sync-field bit splits are not independently checked.
2. **All Intel register names and behaviour:**
   - `PIPECONF`/`TRANS_CONF`, `HTOTAL_A`, `VTOTAL_A`, `PLANE_CTL`, `PLANE_SURF`/`DSPSURF`, `PLANE_STRIDE`, `PIPE_FRMCOUNT`, `GMBUS0`–`GMBUS5`;
   - BAR0 as GTTMMADR;
   - latching at vblank;
   - power wells and DMC.
3. **DDC address 0x50 and retries.**
4. **The hardware paragraph:** transcoders, plane fetch, arming.

Other boxes:

- **Warning box:** everything on Intel hardware is untested.
- **Safety box:** eDP panel power sequencing.

Further notes:

- The "scanline counter" mention in Layer 3 is marked unverified inline.
- **Course forensic "Tearing":** the evidence comes from the course's own flip model (`flipsim.h`), not a real display, and the chapter says so twice.

### F4-47 Research engines (feasibility studies)

Unverified boxes:

1. **The VideoCore IV fill rule, subpixel precision and sample point** (D1 not opened).
2. **The hardware paragraph:** incremental edge evaluation, tiling.

Other boxes:

- **Warning box:** no G4/G6 prototype.
- **Safety box.**

The G4/G6 study drafts in `study_check.in` are teaching examples written by the author.

The top-left rule is described from memory as "the convention in common graphics literature", without a cited source. **Owner decision:** add a citable source, for example a graphics API specification's rasterisation section, through the Source Researcher.

## Sources opened in this build (tier 4, as installed)

- **Linux UAPI headers** from linux-libc-dev 6.8.0-146.146:
  - `linux/kfd_ioctl.h`, `linux/virtio_gpu.h`, `linux/virtio_ids.h`, `linux/pci_regs.h`;
  - `drm/amdgpu_drm.h`, `drm/drm.h`, `drm/drm_mode.h`, and the other `drm/*.h` headers counted by F4-45.
- **ROCm:** `hsa/hsa.h` (libhsa-runtime-dev 5.7.1-2build1).
- **QEMU:** `/usr/share/qemu/trace-events-all`.
- **Images and firmware:** SeaBIOS VGA BIOS images (parsed); NVIDIA GSP firmware files (listed only).
- **This project:** curriculum sections 15 and 16 (a map, not a source) and DRIVERS.html.

Every other source is cited by title with the "Title only — not opened during this build" marking (dossier gate G1 open). Among them:

- **Specifications and standards:**
  - OASIS VIRTIO (GPU device);
  - PCI Firmware Specification;
  - PCI Express Base Specification;
  - VESA E-EDID;
  - HSA specifications.
- **Vendor documents:**
  - LLVM AMDGPU user guide;
  - Intel PRMs;
  - Broadcom VideoCore IV guide;
  - WDK/WDDM documentation.
- **Driver sources and documentation:**
  - Linux DRM documentation and the amdgpu/amdkfd, i915/xe, vc4 and other driver sources;
  - Mesa;
  - NVIDIA open-gpu-kernel-modules and nouveau;
  - Asahi Linux, envytools and open-gpu-doc;
  - SeaBIOS documentation.

## Decisions for the owner

1. **New analogy mappings** in the F4 world ("the school and its visitors"). Proposed and used in these chapters, not yet in the guide's registry:
   - **GPU** = a visiting theatre company with its own crew.
   - **GPU kernel driver** = its interpreter (the registered driver = interpreter mapping).
   - **Command buffer / AQL packet** = a running order.
   - **User-mode queue (ring)** = the order board at the stage door.
     - **Doorbell** = the bell on the board. This is consistent with DR301's NVMe "bells on the trays".
   - **Firmware microcontrollers** = the company's stage managers, who work only from sealed, signed scripts.
   - **Fence** = the "done" stamp.
   - **Display** = the hall's projector, which paints a slide line by line.
     - **Page flip** = changing slides.
     - **Vertical blank** = the dark moment between showings.
     - **Tearing** = swapping the slide mid-projection.
   - **virtio-gpu resource** = a numbered slide in the company's slide store.
     - **Backing memory** = the teacher's own drawing.
   - **Expansion ROM** = the instruction booklet taped to the first crate.
     - **ROM BAR enable** = the reading lamp on the crate.
   - **Feasibility study** = the drama club's report on the scenery lift.

   Please accept or replace them.
2. **Course project options.** G2 (F4-43) and G5 (F4-46) each have a mini-project section with a rubric: 20 points for G2 and 30 for G5. Please confirm the point scales against the course's grading rules.
3. **Word counts.**
   - F4-42 is about 7,400 words of prose, above the L4 target of about 5,000. Exam P's dispatch path and the page-flip path are both there. The owner may split the "same jobs in other drivers" and memory-management subsections into a sidebar.
   - F4-46 and F4-47 (L5) are about 4,100–4,800 words including project material.
4. **Glossary.**
   - `Fence (GPU)` is defined separately from the existing `Memory fence` (SP203/HW203).
   - `EDID` is defined here (F4-43) and linked from F4-46.
   - No slug clashes with other courses' glossary files were found at build time. Courses still being written by other agents may add clashes.
5. **Exam P model answer.** The model answer is F4-42 Layer 3 "The compute dispatch, step by step". Steps 2–7 are inside an unverified box until the Source Researcher opens the HSA specifications and traces ROCm on a GPU.
6. **Hardware.** Moving any G1/G3/G4/G5/G6 part from "untested" to "tested" needs specific machines:
   - a spare machine with an AMD GPU on the ROCm list (G1 trace, G3, G4);
   - an Intel-graphics PC or laptop with an external monitor (G5);
   - a Raspberry Pi 3 (G6).
