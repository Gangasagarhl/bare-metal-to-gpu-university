# DS401 — High-performance networking and RDMA: author notes

These notes cover chapters F5-34 to F5-38, all at level L4. DS401 has 3 credits. Its prerequisites are DS301 and HW205. It maps to curriculum Track F (section 14.1, row "Inter-node networks"), milestone F5 (concepts) and reading-list item 81.

| Course card item | Where it is |
|---|---|
| Lab "a ping-pong over verbs" | F5-36 (Listing 1, real libibverbs; plus a model track) |
| Lab "Soft-RoCE or real RDMA NICs (stated per lab)" | Each lab says what it needs. **None ran on RDMA hardware or Soft-RoCE**: see below. |
| Forensic "RDMA slower than TCP" | F5-37: a configuration dump (path MTU 1024 because Ethernet MTU is 1500; PFC off on the lossless class) plus the university's fabric model |
| Exam P "explain a verbs program line by line" | F5-36 Code walk-through: a line-range table of `verbs_pingpong.cc` that serves as the model answer |
| Project "RDMA-based replicated log append benchmark" | F5-38, section "DS401 course project": brief, requirements, milestones, rubric, model track for learners without hardware |

Each chapter also has its own forensic lab:

- F5-34: Nagle plus delayed ACK on a two-write request
- F5-35: incast capture with a NAK and go-back-N
- F5-36: follower registered without `REMOTE_WRITE`
- F5-37: the course forensic
- F5-38: "kernel bypass made it slower", a sleep in the polling loop

## Environment (as recorded in the logs)

- **Compiler:** g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0. Course flags: `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. The measurement builds use `-O2` without sanitizers and are labelled as such in the logs.
- **Tools:** strace 6.8 and TShark (Wireshark) 4.2.2.
- **Machine:** a shared cloud build container, Linux x86_64, with 4 CPUs visible.
- **RDMA software:** `libibverbs-dev`, `libibverbs1`, `ibverbs-providers` and `librdmacm1t64` are installed, all version `50.0-2ubuntu0.2`. The providers include `librxe` (Soft-RoCE) and `libsiw`.
- **What is missing:**
  - no `/sys/class/infiniband` and no `/lib/modules`, so there is no RDMA device and Soft-RoCE cannot be created;
  - `ibv_get_device_list` returns 0 devices with errno 38;
  - perftest is not installed;
  - no `librdmacm-dev` header.
- **Networking:** every network program uses TCP over plain loopback (127.0.0.1) inside the container. No namespaces are used and nothing leaves the machine.

## Listings run

Every lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All numbers quoted in the prose are from the current `.out` files.

- F5-34, F5-35 and F5-36 were last run before their chapters were finalised.
- F5-37's `run.sh` was rerun once on its own after its final `run_lab.sh`; the chapter quotes those files.
- F5-38 was rerun after adding the `bypass_sleep_O2` step, and the chapter was then written from that run.

**Do not rerun labs without updating the quoted numbers.** Timing results vary from run to run on this shared container.

| Lab | Listing / step | Result |
|---|---|---|
| F5-34 | `paths.cpp` (TCP loopback vs shared-memory mailbox), `nagle.cpp`; run.sh: `paths_O2`, `syscalls_tcp`, `syscalls_shm`, `nagle_trace`, `nagle_nodelay`, `nagle_onewrite` | pass |
| F5-35 | `roce_packet.cpp` + `packet.hpp` (hand-built RoCE v1/v2 frames), `incast.cpp`; run.sh: tshark `dissect`, `decodes`, `incast_capture`, `nak_detail` | pass. The frames are built in C++ and decoded by tshark: **untested on hardware** |
| F5-36 | `model_demo.cpp` + `verbs_model.hpp` (toy verbs model), `replog_fault.cpp`; run.sh: `environment`, `verbs_pingpong` | pass. `verbs_pingpong.cc` compiles and links against real libibverbs, then **exits with code 2 as expected** (no device): **untested on hardware** |
| F5-37 | `bench_tcp.cpp` (perftest-style harness over TCP), `fabric_model.cpp` + `fabric_model.in`; run.sh: `bench_O2`, `fabric_whatif`, `perftest_check` | pass. perftest is not installed: **perftest commands untested** |
| F5-38 | `ring_ipc.cpp` + `spsc_ring.hpp`, `zero_copy.cpp`, `bypass_sleep.cpp`; run.sh: `ring_O2`, `syscalls_socket`, `syscalls_ring`, `bypass_syscalls`, `bypass_sleep_O2`, `bypass_spin` | pass. Shared memory on one machine: the RDMA statements are **untested on hardware** |

The file extension `verbs_pingpong.cc` is chosen on purpose. `run_lab.sh` builds every `.cpp` without extra libraries, so the verbs program is built by `run.sh` with `-libverbs` instead.

## What was verified, and how

- **`infiniband/verbs.h` and `opcode.h` (installed 50.0) were opened.** Checked against them:
  - every identifier used in the chapters (the compiler also checked Listing 1 of F5-36);
  - the enum values quoted: `ibv_mtu`, `ibv_access_flags`, `ibv_qp_state`, `ibv_qp_attr_mask`, `ibv_wc_status`, `ibv_qp_type`, and the opcode values;
  - the quoted comments: the `ibv_get_device_list` and `ibv_create_cq` parameter comments, the `ibv_poll_cq` return comment, the `IBV_SEND_INLINE` reuse comment and the `ibv_fork_init` fork-safety comment;
  - the completion-channel functions named in F5-38.
- **Wireshark 4.2.2 decodes** (tier 4, "what this implementation does"):
  - UDP port 4791 and EtherType 0x8915 are decoded as InfiniBand;
  - field names of BTH, RETH and AETH;
  - AETH syndrome 0x60 is decoded as "Nak, PSN Sequence Error";
  - DSCP 26 is decoded as AF31.
- **All performance numbers come from programs in these labs.** No RDMA hardware numbers appear anywhere.

## Unverified claims (each is in an amber box in its chapter)

**F5-34**
- whether provider data-path verbs make no system calls;
- what the kernel does at set-up;
- the adapter's internals (where queues, doorbells and translation tables live): untested on hardware.

**F5-35**
- UDP port 4791 and EtherType 0x8915 are checked only against Wireshark, not against the RoCE v2 annex;
- the UDP source-port rule, the UDP checksum rule and the ICRC computation (the lab writes a zero ICRC);
- IEEE 802.1Q PFC/ETS clause numbers, the ECN-based congestion-control algorithm and vendor DSCP recommendations;
- what a responder does after a PSN gap;
- the adapter's internal steps: untested on hardware.

**F5-36**
- the `toRtr`/`toRts` magic values (`min_rnr_timer 12`, `timeout 14`, `retry_cnt 7`, `rnr_retry 7`, `hop_limit 1`, `max_rd_atomic 1`) and how to choose the GID index;
- the exact QP state-transition rules, the flush behaviour and the responder's state after an access error. The toy model implements the author's reading of these;
- Listing 1 beyond `ibv_get_device_list`: untested on hardware.

**F5-37**
- all perftest flags and output columns (perftest was not installed);
- the fabric model is a teaching model. Its loss probability (0.002) and in-flight bytes (131072) are chosen values, not measured ones. It does not model pause cost, ECN, timeouts or selective repeat;
- go-back-N as RoCE RC recovery;
- the perftest WRITE latency method, inline sends and pause behaviour: untested on hardware.

**F5-38**
- RLIMIT_MEMLOCK accounting for RDMA registration, the treatment of privileged processes, and ODP;
- why a 1 µs sleep lasts tens of µs: timer slack (a 50 µs default is recalled from memory), wake-up latency and idle-state exit;
- other bypass systems (user-space packet frameworks, user-space NVMe, io_uring) are named from general knowledge;
- all RDMA statements: untested on hardware.

## Sources

Every D-source of the specifications and vendor manuals is cited **title only — not opened during this build (dossier gate G1 open)**:

- InfiniBand Architecture Specification and the RoCE v2 annex (item 81);
- NVIDIA "RDMA Aware Networks Programming User Manual";
- rdma-core manual pages;
- the perftest README;
- IEEE 802.1Q and the IETF DSCP/ECN RFCs;
- Linux man pages for `mlock`, `getrlimit`, `prctl` and `clock_nanosleep`;
- *TCP/IP Illustrated*;
- GPUDirect, UCX and Open MPI documentation.

The Source Researcher needs to confirm releases, chapter titles and tiers.

## Analogy registry proposals (F5: friends in different towns)

New mappings proposed for the registry:

| Analogy | Maps to |
|---|---|
| post office counter and clerk | kernel network stack; system call = queuing at the counter |
| courier with a key to one shelf | RDMA NIC with a memory registration; the key card given to a friend = rkey; the household's own key = lkey |
| checking the shelf / standing at the window | polling |
| courier's receipt in the receipts box | work completion in a completion queue |
| household key ring | protection domain |
| the pair of trays "to send" / "to receive" | queue pair; a slip in a tray = work request; the bell = doorbell |
| one slow phone call before couriers start | out-of-band exchange |
| no empty envelope in the "to receive" tray | receiver not ready (RNR) |
| private courier railway; timetable office; station number; full postal address | InfiniBand; subnet manager; LID; GID |
| couriers on public roads | RoCE |
| painted lane with traffic lights | lossless traffic class with PFC |
| redeliver everything since the lost parcel | go-back-N |
| shelf bolted to the wall; how many shelves you may bolt | pinned memory; RLIMIT_MEMLOCK |
| one letter there and back / a lorry-load | latency test / bandwidth test |
| a nap that always lasts longer than asked | timed sleep in a polling loop |

## Finding for HW205 (F1-53)

F1-53 has unverified boxes that this course has now checked:

- **Checked against the installed header (50.0):** the verbs names `ibv_open_device`, `ibv_alloc_pd`, `ibv_reg_mr`, `IBV_ACCESS_*`, `ibv_create_cq`, `ibv_create_qp`, `ibv_post_send`, `IBV_WR_RDMA_WRITE`, `IBV_WR_RDMA_READ`, `IBV_WR_SEND`, `ibv_post_recv`, `ibv_poll_cq` and `IBV_WC_REM_ACCESS_ERR`.
- **RNR error name:** `IBV_WC_RNR_RETRY_EXC_ERR` exists in the same header.
- **Port 4791:** what Wireshark 4.2.2 decodes as RoCE v2. This is a tier-4 check, not the annex.

The HW205 author may narrow those boxes and link to F5-36 and F5-35. I did not edit HW205, because it is outside my folders.

## Decisions for the owner

1. **RDMA hardware or Soft-RoCE for the labs.**
   - The course card says "Soft-RoCE or real RDMA NICs (stated per lab)". The build container has neither: no kernel modules, no `/sys/class/infiniband`.
   - Every RDMA program is therefore compiled, linked and run to its honest "no device" exit (F5-36), or replaced by a model.
   - Before release, someone needs to run F5-36 Listing 1 and the F5-37 perftest plan on two hosts, then record the results with machine, adapter, firmware and versions.
2. **perftest is not installed.** Install the package in the build image, or accept the unverified flag list in F5-37.
3. **The course project has no reference solution.** It was not built or run, and no `_keys/` notes were written, because `_keys/` is outside my folders. The model track (toy verbs plus shared-memory rings) makes it gradable without hardware. Please confirm that this is acceptable.
4. **The ICRC is not computed.** F5-35 writes a zero ICRC, and tshark does not judge it. Computing it needs the annex's masking rules, which are unverified.
5. **Loopback without namespaces.** DS201 isolated its network programs with `unshare -n`, but DS401's TCP programs use plain loopback on 127.0.0.1. They make no external connections and need no root. Please say whether the DS201 convention should apply here too.
6. **Run-to-run timing variation.** Small-message latency medians vary a lot on this container. Two examples:
   - F5-37: α was 12.88 µs in one run and 19.95 µs in another.
   - F5-38: the socket median was 4.50 µs in the sanitizer run and 24.27 µs in the `-O2` run.

   The chapters teach this explicitly and quote minima and system-call counts where those are stable. A rebuild will print different numbers, so the prose must be refreshed together with the outputs.
