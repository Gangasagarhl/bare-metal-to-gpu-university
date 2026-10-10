# DR404 — Virtualization: author notes

Chapters F4-38 to F4-41. Level L4, 4 credits. Curriculum section 12 (Virtualization), milestones V1–V3.
Labs are in `university/labs/F4-38` to `university/labs/F4-41`.

Each lab passes `university/labs/run_lab.sh university/labs/<ID>` with exit code 0. All four were re-run at the end of this build (2026-10-09). The prose was then checked against the outputs of that last run. Numbers that change from run to run are given as ranges seen across this build's runs, not as single values. These are host CPU percentages, exit totals of the polling guests, and calibrated TSC rates.

Every fragment passes the local checks:
- html.parser balance;
- ids prefixed with the chapter id;
- no URLs;
- no `<script>`;
- all 21 sections plus Answers and the forensic key;
- source references resolve;
- none of the banned words.

`python3 university/build/build.py` reports no PROBLEM line for F4-38 to F4-41 or for the DR404 glossary, and none of its broken-link lines is a link from these chapters. The PROBLEM lines it still prints belong to other courses.

## Shared code across the four labs

- **F4-39 holds the shared kernel code.**
  - Files: `boot.S`, `kio.*`, `intr.*`, `kernel.ld`, `build_kernel.sh`.
  - Helper scripts: `dr404lib.sh`, with `rec`, `kbuild` and `qrun` (QEMU exit status 1 = pass, 3 = fail, 0 = reset, 124 = time limit), and `cputime.py`.
  - F4-40 and F4-41 build from these files by relative path (`../F4-39`, `../F4-40`). The folders therefore depend on each other: do not move one without the others.
- **F4-41's guest is F4-39's `goodguest` kernel, built from unchanged sources.** It is embedded in the hypervisor image with `.incbin` (`guest_image.S`).
- **Toolchain** (recorded in each `.log`): g++ 13.3.0, GNU ld 2.42, objdump 2.42 and QEMU 8.2.2 with TCG.
  - Hypervisor runs use `-cpu qemu64,+svm,+npt`.
  - The build container has no `/dev/kvm`. It is itself a KVM guest, and neither VMX nor SVM is offered to it (`F4-39/hostguest.out`).
  - Host programs are built by `run_lab.sh` with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.

## Listings run

**Nothing was run on real hardware.** Every QEMU run is untested on hardware.

| Chapter | Run | Status |
|---|---|---|
| F4-38 | trapemu (host) | pass: runs 1, 2, 3 and 5 OK. Run 4 shows LOST UPDATES by design (it is the chapter's forensic evidence) |
| F4-38 | walk2d (host) | pass: 8, 15, 24, 19 and 35 reads, each equal to (n+1)(m+1)−1 |
| F4-39 | build | pass |
| F4-39 | detect | pass (exit 1): signature TCGTCGTCGTCG. Untested on hardware |
| F4-39 | detect_nohv | pass (exit 1): "none", with `-cpu qemu64,-hypervisor` standing in for bare metal |
| F4-39 | idle_hlt | pass (exit 1): 4–7 % of one host CPU across runs |
| F4-39 | forensic_poll | pass (exit 1). This is the forensic evidence: 82–92 % of one host CPU |
| F4-39 | mono | pass (exit 1): 10M reads, 0 backwards (TSC clock, 1 vCPU, host not loaded) |
| F4-39 | hostguest (host) | pass: the build container is a KVM guest; it shows the KVM features, kvm-clock, the virtio devices, and VMX no / SVM no |
| F4-39 | pvclock_model (host) | pass: a torn read goes backwards and the seqlock reader rejects it (deterministic) |
| F4-40 | build | pass |
| F4-40 | hello | pass (exit 1): 134 exits (132 IOIO, 1 CPUID, 1 VMMCALL). Untested on hardware |
| F4-40 | triple | pass (exit 1): SHUTDOWN is reported, then a new guest runs |
| F4-40 | invalid | pass (exit 1): 4 invalid states are refused with code ffffffff, then a valid run |
| F4-40 | hlt | pass (exit 1): 268 exits (200 HLT), a few per cent of the CPU |
| F4-40 | forensic_busy | pass (exit 1). This is the course forensic, "The guest that runs slowly": about 180k–215k exits, almost all on PIT ports 0x43 and 0x40 |
| F4-40 | exitdecode (host) | pass: SVM and VMX codes decoded with Linux's tables |
| F4-41 | build | pass (guest, hypervisor, probe, forensic build) |
| F4-41 | npt_probe | pass (exit 1): with guest paging off the access is NOT translated; with guest paging on there is an NPF |
| F4-41 | guest_detect | pass (exit 1): the F4-39 kernel finds "DR404-tinyHV" |
| F4-41 | guest_hlt | pass (exit 1): 221 HLT exits and 221–222 injected ticks |
| F4-41 | guest_poll | pass (exit 1): about 140k–175k exits |
| F4-41 | guest_mono | pass (exit 1): 10M reads, 0 backwards |
| F4-41 | guest_npf | pass (exit 1, expected NPF): gPA 0x40000000, error code 0x100000004, guest stopped |
| F4-41 | forensic_entry32 | **expected reset (QEMU status 0)**: with the 32-bit entry the guest runs the hypervisor's own `_start` and zeroes host .bss, and the host triple-faults |

## Claims not verified in this build (boxes in the chapters)

- **F4-38: feature names from the curriculum.** The interrupt-virtualization names (APIC virtualization, posted interrupts, AVIC, GICv4, AIA) and the confidential-computing names are taken from the curriculum's list. The same goes for the VMX, SVM, EL2 and H-extension instruction and register names in "How the hardware actually does it". POPF's behaviour at lower privilege is from memory of the SDM.
- **F4-39: kvmclock record.** The record's layout and fields, the registration MSR and the meaning of the stable bit are from memory of "KVM-specific MSRs". Listing 3 is a model of them.
- **F4-39: other hypervisors' signatures.** The Hyper-V, Xen and VMware signatures and interfaces come from curriculum table 12.3.
- **F4-39: CPUID and HLT exits.** "CPUID always exits on VMX" and KVM's handling of HLT are from memory.
- **F4-40: AMD and Intel manual details**, all written from memory of the manuals. What QEMU 8.2.2 shows is only that its model agrees with them.
  - AMD:
    - all VMCB offsets;
    - the intercept-bit order;
    - the IOIO EXITINFO1 format;
    - the MSR numbers (EFER, VM_CR, VM_HSAVE_PA);
    - the VMRUN consistency checks;
    - the segment attribute format.
  - Intel:
    - the split between VMfailValid and VM-entry failure;
    - the VM-instruction error numbers.
- **F4-41: nested paging details**, from memory:
  - the NPF error-code bits 32 and 33;
  - the rule that nested walks are user accesses;
  - the EVENTINJ format;
  - the NP_ENABLE and N_CR3 offsets;
  - the CPUID bit for nested paging.
- **F4-41: nested paging with guest paging off.** The chapter assumes that "nested paging applies even when guest paging is off". QEMU 8.2.2 TCG does not behave that way (R6). The chapter says that we believe this is a QEMU limitation, and that this is not settled.

**Verified in this build** from opened sources:
- the SVM exit codes, VMX exit reasons and `VMX_EXIT_REASONS_FAILED_VMENTRY` (`<asm/svm.h>`, `<asm/vmx.h>`);
- the KVM signature, leaf numbers and feature bits (`<asm/kvm_para.h>`);
- the virtio device ids (`<linux/virtio_ids.h>`);
- the event-index bit 29 (`<linux/virtio_ring.h>`, read and copied, because the header does not compile as C++).

All of these come from linux-libc-dev 6.8.0-146.

Every book, paper and manual in the Sources sections is title only, so dossier gate G1 is open for all of them.

## Findings of this build that the owner may want to know

1. **QEMU 8.2.2 stores SVM's VMRUN failure code as 0x00000000FFFFFFFF.** The header defines it as −1 (64 bits), and QEMU writes only the low 32 bits. The hypervisor compares the low 32 bits, and F4-40 says so. Behaviour on hardware is unchecked.
2. **QEMU 8.2.2 TCG did not apply nested paging while the guest had paging off.** `npt_probe` shows it, and the problem made the first F4-41 design crash the host. The workaround is to enter the guest at its 64-bit `long_mode` entry, with page tables built by the hypervisor. This became the F4-41 forensic lab. Someone should check a newer QEMU and the APM.
3. **The guest's calibrated TSC under F4-41 is wrong.** It reads 2.10–2.48 GHz, while the host measures about 2.05–2.10 GHz. The cause is late and dropped virtual ticks. The chapter uses this as its "lost ticks" teaching point.
4. **The build container is a KVM guest** (signature KVMKVMKVM, kvm-clock offered, virtio devices). F4-38 and F4-39 use it as a live example.

## Deviations from the course card and the milestones

- **V1 is partly done.**
  - Done: detection is implemented and tested under QEMU/TCG, plus the "none" case with the bit hidden. Idle behaviour is measured, and 10M monotonic reads pass using the TSC.
  - **Not built:**
    - the kvmclock driver (only a model of it exists);
    - the Hyper-V reference TSC page;
    - the virtio-rng and virtio-console drivers;
    - B8's sleep test.
  - **Not run:**
    - under QEMU/KVM;
    - under Hyper-V;
    - on bare metal;
    - with several vCPUs.
  - These steps are listed in the F4-39 lab.
- **V2 (the course project) is done on AMD SVM only, under QEMU's emulation.**
  - All three acceptance tests pass there.
  - Missing:
    - control-register intercepts (now a lab step);
    - a VMX/VT-x version (TCG does not emulate VMX, and there is no KVM to nest under);
    - development under nested virtualization on KVM, which the milestone asks for.
- **V3 (the stretch project) is partly done.**
  - Done: NPT, injection of PIC timer interrupts, CPUID filtering, a kernel booting as a guest, and the NPF acceptance test.
  - **Not done:**
    - the virtio-blk back end;
    - MSR filtering;
    - a local APIC timer;
    - the B-track kernel passing B1–B9 and B15 as a guest;
    - the AArch64 and RISC-V versions.
  - Some of these are lab steps; the virtio-blk back end is the mini-project.
- **Forensic labs.** The course card's forensic, "The guest that runs slowly", is F4-40's forensic lab. The other three chapters each have their own forensic lab:
  - F4-38: lost updates caused by an unprivileged sensitive instruction;
  - F4-39: idle polling seen from the host;
  - F4-41: a host crash from a guest whose addresses were not translated.
- **Exam P** ("explain a VM-entry failure from its documented error code") is F4-40's Worked example 1 and Check yourself question 5.

## Analogy proposals (F4 world: the school)

The registered mapping is used throughout: "Virtual machine / hypervisor = a school inside a school, with the real principal above", including its stated break. These new mappings are **proposals** for the analogy registry:

| Concept | Proposed analogy | Where it breaks |
|---|---|---|
| Hypervisor-present bit and signature | The door plate: "Wing B, property of the main school" | The plate can be hidden or lie; detection is only a performance hint |
| Paravirtual clock record and seqlock | The building's master clock with a repeater in each wing; a "being updated" sign on the timetable board | The clock is read from the guest's own memory, not walked to |
| VMCB / intercepts | The written agreement: "call me when you touch the bell, the boiler or the front door" | Hardware enforces it; the guest cannot opt out |
| Exit counts by reason | The real principal's ledger of calls | — |
| Nested page table | The building's master plan for the guest school's rooms | It is consulted at every step of the guest's own lookup (2D walk) |
| Event injection / virtual PIC | The real principal rings the small school's bell for them | Ticks can be late or lost when nobody is in |

## Decisions for the owner

1. **SVM only.** Should a VMX/VT-x version of F4-40 be required? It needs a KVM host with nested VMX or Intel hardware, and neither is available to this build.
2. **Can the V2 project be graded on QEMU TCG alone**, or must it be on nested KVM or hardware, as the milestone says?
3. **The 64-bit guest entry in F4-41** depends on a QEMU behaviour that we believe is a QEMU limitation. If a newer QEMU or the APM settles the question, F4-41's Layer 3 and its forensic lab need revising.
4. **V1's kvmclock driver and the virtio-rng and virtio-console drivers** are left to the student (lab steps) and are not built here. Is that acceptable, or should the next build add them? They need KVM to be tested.
5. **The labs depend on each other** (F4-40 and F4-41 build from `../F4-39` and `../F4-40`). Keep this, or copy the shared files into each folder?
