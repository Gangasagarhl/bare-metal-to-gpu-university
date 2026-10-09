# DS304 — Server hardware and management: author notes

Chapters F5-28 to F5-33, level L3–L4. Labs are in `university/labs/F5-28` to `university/labs/F5-33`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All six were run again at the end of this build, on 2026-10-09 (see "Final run").

## Toolchain and local evidence (as recorded in the logs)

- **Host compiler:** g++ 13.3.0 with the course flags of `run_lab.sh` (every `*.cpp`). `F5-32/work_host.cc` is built by `run.sh` with `-O2` and no sanitizers, so that its timings compare with the guest's.
- **UEFI programs** (F5-28 `numa.cc`, F5-29 `kcs.cc`, F5-31 `netboot.cc`, F5-32 `guest_cpuid.cc`): Ubuntu clang 18.1.3 with `--target=x86_64-unknown-windows -ffreestanding -fno-exceptions -fno-rtti -mno-red-zone -fno-stack-protector -O2 -Wall -Wextra -Wpedantic -Werror -I ../F3-10`, then `lld-link /subsystem:efi_application /entry:efi_main /nodefaultlib` (LLD 18.1.3).
  - They reuse `F3-10/efi.hpp`, `console.hpp` and `qemu_run.py` read-only. Nothing in OS301's folders was changed.
  - `guest_cpuid.cc` defines `extern "C" int _fltused = 0;` because it uses floating point.
- **Arm bare metal** (F5-30 `bmc_hello.cc`): clang 18.1.3 `--target=armv7a-none-eabi -mcpu=cortex-a7`, `ld.lld -T bmc_hello.ld`.
- **QEMU:** 8.2.2 (Debian 1:8.2.2+ds-0ubuntu1.18).
  - `qemu-system-x86_64 -machine q35`: NUMA (`-numa`), SMBIOS (`-smbios`), the simulated BMC (`ipmi-bmc-sim` + `isa-ipmi-kcs`), and user-mode networking with built-in DHCP/TFTP (`-netdev user,tftp=,bootfile=`).
  - `qemu-system-arm -machine ast2600-evb`.
- **Firmware:** OVMF 2024.02-2ubuntu0.10 (`OVMF_CODE_4M.fd`, a copy of `OVMF_VARS_4M.fd` per run).
- **Local file opened as evidence:** `/usr/include/linux/ipmi_msgdefs.h` from linux-libc-dev 6.8.0-146.146 (source L1 of F5-29). It confirms netfn App 0x06/0x07, Storage 0x0a/0x0b, Get Device ID 0x01, Add SEL Entry 0x44, completion codes 0x00, 0xc1 and 0xff, and BMC slave address 0x20.
- **Build container:** the sysfs topology shows 1 NUMA node and 4 CPUs in 1 package. CPUID reports a hypervisor with signature "KVMKVMKVM".
- **Not available:**
  - No `/dev/kvm`.
  - No OpenBMC image and no Yocto build environment.
  - No real server, BMC, PSU, fan or network.
  - No internet.

## Listings run

**Nothing was run on real hardware.** Every QEMU run's log carries a "hardware: untested on hardware" line.

| Chapter | Run | Status |
|---|---|---|
| F5-28 | topology, placement, qemu_numa_help | pass |
| F5-28 | numa_2s, numa_flat (exit 33), check_numa | pass. SRAT/SLIT from QEMU's two-node machine; "no SRAT" without `-numa` |
| F5-28 | forensic_numa (exit 33) | pass: forensic evidence, a node with 0 KiB of memory (memory-less node) |
| F5-29 | bmc_props, redfish, power_audit | pass (power_audit is the forensic evidence generator) |
| F5-29 | kcs | pass. Exit 0 is by design: Chassis Control "power down" switches the VM off before the program's own exit (33) |
| F5-29 | check_kcs | pass: 5 PASS plus a NOTE (device ID mismatch, see decisions) |
| F5-30 | bmc_machines, bmc_mtree, objects, forensic_fans | pass |
| F5-30 | bmc_hello | pass. Exit 124 is by design: the program halts and the machine has no exit device, so the 10 s limit ends QEMU; the UART file is the result |
| F5-31 | qemu_net_help, netboot_a, netboot_b (exit 33), inventory, check_netboot | pass: PXE boot over QEMU's DHCP/TFTP on two servers; all checks PASS; the UUID line reports the byte-order difference |
| F5-31 | forensic_config, forensic_pxe | **expected fail**: forensic_pxe exits 124 (stopped after the second failed boot option); TFTP error PXE-E23 from a lower-case boot-file name |
| F5-32 | hvdetect, guest_tcg, guest_nohv (exit 33), work_host, check_guest | pass |
| F5-32 | accel_kvm | **expected fail**: exit 1, "Could not access KVM kernel module" (no `/dev/kvm`) |
| F5-32 | forensic_accel | pass (exit 33): forensic evidence, `-accel kvm -accel tcg` falls back to TCG |
| F5-32 | consolidate | **expected exit 3**: one VM (batch2) unplaced, the worked example's result |
| F5-33 | fan_loop, availability, night | pass (model and arithmetic; night is the course forensic's evidence generator) |

Untested on hardware: every chapter. Untested at all: booting an OpenBMC image in QEMU (F5-30), everything that needs KVM (F5-32; the V1 cases QEMU/KVM, Hyper-V and bare metal), the out-of-band IPMI-over-LAN and Redfish-over-HTTPS paths (F5-29), UEFI HTTP boot (F5-31), and every real sensor or power measurement (F5-33).

## Unverified claims (boxes "Not verified — check before relying on this")

Every D-source is cited by title only (dossier gate G1 open). Each chapter has one unverified box, plus "untested on hardware" statements in its hardware section and lab.

- **F5-28:** product-specific facts: memory channels per socket, DIMMs per channel, population rules, inter-socket link names and speeds, real SLIT values, measured local and remote latency. The SRAT/SLIT/SMBIOS offsets are confirmed only by agreement with QEMU's `-numa` options.
- **F5-29:**
  - All Redfish names (paths, `@odata.id`, `Members`, `Actions`, `ComputerSystem.Reset`, `ResetType` values, `Thermal`).
  - The IPMI details not in L1: KCS control codes and status bits, the Chassis netfn and its commands, Get Self Test Results, Get SEL Info/Entry, the SEL record layout and SMBIOS type 38 offsets. These are supported only by QEMU's acceptance.
- **F5-30:**
  - The OpenBMC-in-QEMU commands (not run).
  - D-Bus paths and property names, U-Boot/BitBake/image names, and the service names in the role table.
  - The meaning of the SCU word 0x05030303, and the UART register layout.
- **F5-31:**
  - DHCP/PXE options, TFTP details and HTTP boot.
  - The SMBIOS offsets and the UUID byte-order rule (consistent with R4).
  - Device-path node numbers (consistent with OVMF's text).
  - The PXE error-code meanings.
  - The cause of the ~64 s gap before PXEv6.
- **F5-32:**
  - Everything needing KVM.
  - The QEMU fallback list (behaviour confirmed by the run, not by documentation).
  - The Hyper-V/Xen/VMware signatures, "CPUID always exits under VT-x", pre-copy migration, SR-IOV, page merging and nested paging.
- **F5-33:**
  - PMBus, threshold names and SEL event wording.
  - Power restore policy values and the order of protection levels.
  - Aisle practice and inlet ranges.
  - "All power becomes heat".
  - All model and exercise numbers are labelled as such.

## Decisions for the owner

1. **OpenBMC in QEMU (course card: "if the dossier confirms a QEMU machine for it").**
   - The machine exists in this build's QEMU 8.2.2: 25 BMC machines are listed, among them `ast2600-evb`, `romulus-bmc` and `witherspoon-bmc`.
   - No OpenBMC image could be built or fetched here. F5-30 therefore runs a bare-metal program on `ast2600-evb` and studies the architecture with a C++ model; the OpenBMC boot is an optional, untested lab step.
   - To complete it, provide a prebuilt OpenBMC image (for example for romulus or an AST2600 board) in the lab environment.
2. **QEMU's `ipmi-bmc-sim` ignores `device_id`.**
   - With `device_id=0x42`, Get Device ID reports 0x20, which is the property's default ("default: 32" in QEMU's help). A run during authoring with `slave_addr=0x24` still reported 0x20, while SMBIOS type 38 followed the new address.
   - The chapter keeps this as a "compare configuration with what the device reports" lesson and the check prints a NOTE, not a FAIL. The cause (QEMU's `hw/ipmi/ipmi_bmc_sim.c`) was not opened. Confirm, or file it as a QEMU behaviour.
3. **Get Self Test Results returns 0xC1 (invalid command)** on QEMU's simulated BMC. It is used in F5-29 as the completion-code lesson.
4. **KVM is unavailable.**
   - Milestone V1 is covered only for QEMU/TCG, plus the container itself, which reports "KVMKVMKVM" from an unidentified KVM-based monitor.
   - The F5-32 forensic lab uses the real TCG fallback as its injected fault.
   - A lab machine with `/dev/kvm` is needed for the QEMU/KVM case; Hyper-V and bare metal need other machines.
5. **F5-32 timings vary between runs**, because the container is shared. Integer TCG/host ratio: 1.2× in one run, 1.8× in the final run. Floating point: 5.7× and 6.4×.
   - The chapter quotes the final run and the earlier one, and tells students to use ratios.
   - If `build.py` re-runs the labs, the quoted numbers in F5-32 (Layer 3, the lab's expected observations, the summary and the forensic key) may no longer match the inserted output.
   - The same applies to F5-31's forensic time stamps (7.4 s, 71.5 s, 72.0 s) and F5-29's SEL time stamp, which the text describes without quoting.
6. **`consolidate` exits with 3 by design** (one VM unplaced). `run_lab.sh` accepts it because the log records the exit code. Change the program to return 0 if the owner prefers that every listing exit 0.
7. **SMBIOS UUID byte order (F5-31).** The raw bytes show the first three fields byte-reversed relative to the `-smbios uuid=` option. The inventory keeps raw bytes and the check reports the difference; converting it is the mini-project. Decide whether the course project requires the standard text form (recommended).
8. **QEMU's SMBIOS type 17 does not follow the NUMA layout** (F5-28: one "DIMM 0" of 512 MiB for two nodes). This is noted in F5-28 as a reason to trust SRAT over SMBIOS for locality.
9. **Forensic packs that are simulations**, labelled as such in each chapter:
   - F5-29: `power_audit.cpp`
   - F5-30: `forensic_fans.cpp`
   - F5-33: `night.cpp`, the course forensic "The server that reboots at night"
   - The F5-31 and F5-32 forensic packs are real QEMU runs with injected faults.
10. **The P exam ("plan the boot and management path for a rack")** is not written as a separate file. F5-31 Layer 3 ("From two servers to a rack") and its mini-project (the course project, with a one-page rack plan) prepare for it. The owner may want an exam paper in the course folder.
11. **Banned-word checks:**
    - F5-31 uses "Trivial File Transfer Protocol", the protocol's name. It is kept.
12. **Sources needing the Researcher:**
    - DMTF Redfish (the registry records "403 Forbidden").
    - The IPMI v2.0 specification, OpenBMC documentation, the PXE specification and the RFCs.
    - ASHRAE TC 9.9.
    - A reliability textbook (D6 of F5-33 is a placeholder: "chosen by the Source Researcher").
    - A server vendor's hardware manual (D2 of F5-33).
    - F5-29 D5 is the curriculum used as a map (AH-4) and must be replaced.
13. **Chapter length:** about 5,200 to 6,500 words each, including tables, against a target of about 4,000 for L3. The extra is mostly in the lab, forensic and answer sections.

## Analogy proposals (F5, friends in different towns)

| Proposal | Chapter |
|---|---|
| NUMA node = a town with its own library; the interconnect = the bridge between towns; SLIT distance = the travel-time sign on the road | F5-28 |
| BMC = the friend in the gatehouse of a holiday cottage, with his own key, phone line and battery lamp; IPMI = numbered phone codes; Redfish = a written form with named boxes | F5-29 |
| OpenBMC daemons = the gatehouse team who never phone each other but pin notes on one shared notice board (D-Bus) | F5-30 |
| network boot = a new neighbour with an empty house who asks the town hall (DHCP) for an address and the parcel's name, then collects it at the depot (TFTP) | F5-31 |
| hypervisor host = a guesthouse whose flats each look like a whole house; overcommit = more beds than rooms | F5-32 |
| power and cooling = a house's water mains and ventilation; redundancy = a second pipe from another street; MTBF/MTTR = how often a pipe bursts and how long the plumber takes | F5-33 |

## Glossary

`glossary.json` has 36 four-part entries, generated from the chapters' jargon boxes. Each entry's source is the first claim tag of its definition.

- **Not redefined, linked instead:**
  - BMC, IPMI, Redfish, Out-of-band management (OS305)
  - NUMA, First-touch policy (HW203)
  - SMBIOS, OVMF, UEFI (OS301)
  - UEFI application (SP301/OS301)
  - DHCP (DS201)
  - Hypervisor, Hypervisor-present bit, Hypervisor signature, Steal time, Balloon device (DR404)
  - virtio (DR301)
  - IOMMU, I2C (HW204)
  - Availability (DS301)
  - Thermal throttling, Thermal zone and trip point (HW205)
  - Packing and spreading (DS303)
- The F5-29 jargon box repeats the OS305 definitions of BMC, IPMI and Redfish, and says so.
- "Socket (processor package)" is a separate term from DS201's network "Socket". "NUMA node (proximity domain)" warns against confusion with a distributed-systems node.

## Final run

All six labs were run again with `run_lab.sh` at the end of this build, on 2026-10-09. All six returned status 0. F5-31 takes about 85 s, most of it in the forensic boot's time-outs.

`python3 university/build/build.py` reports no PROBLEM line for DS304's chapters, glossary or labs. The remaining PROBLEM lines (47 in the last build) are broken links of other courses.

Runs whose output varies from run to run:

- F5-29: kcs (the SEL time stamp is host time)
- F5-31: forensic_pxe (time stamps)
- F5-32: forensic_accel, work_host (timings)
- All runs: the `date:` lines of the logs
