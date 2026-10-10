# DS403 Distributed operating systems: author notes

Chapters: F5-44, F5-45, F5-46, F5-47 (L4, F5-47 L4–L5). Course card: guide section 5.
Maps to: Curriculum C9 (e1000 driver reused as the cluster network), section 7.1 row
"9P and virtio-fs", goal 1 "distributed computers in servers", guide 11.6 MP2 (core).

Build container facts used: QEMU 8.2.2 (TCG, no KVM), g++ 13.3.0 (32-bit freestanding
kernel + host programs with ASan/UBSan), GNU ld, TShark 4.2.2, Python 3. No real network,
no second machine, no real NIC: every cluster run is three QEMU PCs joined by the lab's own
`labswitch` program over loopback UDP (`-netdev dgram`). All runs are **untested on
hardware**; every node `.log` carries a `hardware:` line saying so.

All labs were re-run at the end of the build (after a container restart interrupted the
first run). The cluster runs are not deterministic (three emulators share the host's
processors; the switch timestamps host time), so tick and millisecond values quoted in the
prose were updated to match the final `.out` files. **If a lab is re-run, the prose values
listed below under "Run-specific numbers" must be re-checked**; the acceptance checks
(`accept.out`, `check.out`, `splitbrain_check.out`) are designed to pass on any run.

## Shared lab infrastructure

- `labs/F5-44/` holds the DS403 kernel core (boot.S, kernel.ld, k.cc, e1000.cc, msg.cc,
  member.cc), the switch (`labswitch.cc`) and `lablib.sh` (build flags, random UDP port
  range per run, `cluster` helper, run records). F5-45, F5-46 and F5-47 reuse them by
  relative path (`../F5-44/...`). Do not move F5-44's folder without updating the others.
- Binaries and disk images are deleted at the end of every `run.sh`.

## F5-44 One computer made of many: the single-system image idea

Listings run (all pass): `availability.cpp` (exit 0); `run.sh`: build, ssi_n1..n3 (QEMU exit
33 = planned halt via isa-debug-exit), switch, accept (6 PASS), forensic flap_n1..n3,
flap_switch, flap_summary.

Unverified boxes:
1. History of SSI / distributed OS systems (LOCUS, V, Sprite, Amoeba, Chorus, Mach, Plan 9,
   MOSIX, Linux SSI projects) and the attribution of "A Note on Distributed Computing" (Waldo
   et al.): from memory.
2. EtherType 0x88B5 as a local-experimental value: not checked against IEEE 802 / registry.
3. Every e1000 register offset and bit in e1000.cc: from memory of the Intel 8254x SDM;
   confirmed only by working on QEMU's model.
4. PIT input frequency (~1.193182 MHz) and latch/mode-2 behaviour: not read from the 8254
   datasheet; one emulated measurement only (~10.0 ms per tick).
5. Untested on hardware (whole chapter).

Run-specific numbers in prose: node 1 / node 2 drop node 3 at ticks 651 / 646 (Figure 2,
Layer 3); 600 ticks = 9,610 − 3,615 = 5,995 ms on the switch clock; switch heartbeat counts
(60 from node 3); mistakes item "remote compute 0 ticks, round trip 0 ticks" vs 15 ms on the
switch (RUN_REQ 5,110 ms, RUN_REP 5,125 ms) and Check-yourself question 8 with its answer;
forensic key: 56/62/67 view lines, departure ticks 172/232/292, 593/647/710/770,
597/651/714/772.

Analogy proposal (not in the guide's F5 registry): single-system image = a club whose
members in four towns answer every caller as if they were one clubhouse.

## F5-45 Lessons from Plan 9 and Amoeba

Listings run (all pass): `capability.cpp`, `namespace.cpp` (host models, exit 0);
`run.sh`: build, session (QEMU exit 33), check (10 PASS), forensic (exit 33; the walk to
"Hello.txt" fails with Rerror ENOENT, which is the intended evidence).

Unverified boxes:
1. Plan 9 and Amoeba history and design details (Bell Labs, terminal/CPU/file server split,
   bind/mount/union directories; Amoeba at VU/CWI, Bullet server, FLIP, capability layout):
   from memory of the papers.
2. Legacy virtio register offsets, split-ring layout, device id 0x1009, mount-tag feature
   bit and config layout: from memory of the OASIS VIRTIO spec; confirmed only by working on
   QEMU 8.2.2.
3. Untested on hardware (virtio-9p has no physical counterpart).

Run-specific number in prose: the qid version of hello.txt quoted in Layer 3 ("versions
such as 1791597927"); it comes from the host file's metadata and changes on every run.

Analogy proposals: 9P = letters asking the library friend to read you a page; fid = a
numbered bookmark the library keeps for you; Amoeba capability = a tamper-proof club card.

## F5-46 Your OS on many machines: network boot and remote file systems

Listings run (all pass): `run.sh`: build, netboot (tshark decode of QEMU filter-dump of node
2's boot NIC), dfs_n1..n3 + dfs_switch (exit 33), accept (7 PASS), forensic stale_n1..n3 +
stale_switch (node 2 with `cache=naive`). DHCP and TFTP are done by QEMU's iPXE ROM and
QEMU's user-mode network, not by the learner's kernel (said in the chapter); the C9 DHCP/TCP
acceptance tests are not run here.

Unverified boxes:
1. Capture interpretation beyond what it shows (iPXE ProxyDHCP wait, TFTP 512-byte default,
   blksize/tsize RFCs, QEMU user-net default addresses, DHCP file field / option 67).
2. NFS and AFS characterisations (stateless server, close-to-open, duplicate-request cache;
   whole-file caching, session semantics, callbacks).
3. Boot timings (one emulated run; ~3.2 s first Discover to TFTP) and the spanning-tree
   PXE-timeout remark: from memory, untested on hardware.
4. Untested on hardware (whole lab).

Run-specific numbers in prose: Figure 1 times (0.000, 1.027, 3.068, 3.163, 3.219 s), the
56 ms between the two TFTP read requests; node 2 drops node 1 at tick 961; switch lines at
15,630 / 20,174 ms; forensic key: server tick 435, node 2 view drop at 969, healthy DFS
lines at 15.6 / 20.2 / 22.3 s, forensic node 3 requests at 14.9 s.

Analogy proposals: network boot = a new friend who arrives with nothing and is posted the
club's rulebook; cache revalidation = phoning the library to ask "is page 3 still edition
2?" before reading your photocopy. Source D10 (AFS and NFS papers) is not in the guide's F5
source registry: proposed addition.

## F5-47 A cluster service layer: membership, consensus, scheduling

Listings run: `quorum.cpp` (pass); `run.sh`: build, sched_n1..n3 + sched_switch (exit 33),
split_n1..n3 + split_switch (Raft with partition {1}|{2,3} from 6 to 16 s), accept (all
PASS: one OK in the partition run, all three nodes agree on lock=n3), splitbrain_n1..n3 +
splitbrain_switch + splitbrain_check. **splitbrain_check is an expected failure**: its
log records `result: expected-fail: safety violated, as the forensic lab needs` (two
clients told OK); run.sh fails if the violation does not happen.

Unverified boxes:
1. Raft paper section numbers in raft.cc comments and the "figure 8" scenario: from memory.
2. "Persistence dominates commit latency on real machines": general experience, no
   measurement.
3. Untested on hardware.

Run-specific numbers in prose: Figure 2 (node 2 leader at 635, CAS at 848, node 3 vote at
665, told OK at 904); election start at node 1's tick 163; scheduler re-assignment of job 3
at node 1's tick 724 (index 12), applied on node 2 at 790; node 1 steps down at 1300;
forensic key: node 2 applied CAS at 828, node 3 told OK at 905, view ticks 488/491/1139/1142
(node 1), 435/1111 (node 2), 492/1177 (node 3).

All analogies used are registered (pizza vote, friend who writes the order, shared
notebook, phone lines down, organiser assigning chores).

## Decisions for the owner

1. Proposed F5 analogy mappings listed above (F5-44, F5-45, F5-46): accept into the
   registry or ask for replacements.
2. Proposed source D10 for F5-46 (AFS / NFS papers) and the Borg paper (F5-47 D3), FLP and
   "Paxos Made Simple" if not already registered for DS403.
3. Nondeterministic run values in prose: keep them (updated on every re-run, list above),
   or switch the chapters to phrasing that cites the `.out` without exact ticks. A future
   option is QEMU `-icount` for the guest clocks, but the switch's host clock would still
   vary.
4. The lab kernel is shared across chapters via `labs/F5-44/`; MP2 (the course project)
   expects learners to port it to their own MP1 kernel.
5. Practical exam P (design review) is described in F5-47 per guide 11.4; reviewer scenario
   list and time limit are left to the owner.
6. The glossary entry "TFTP" also exists in DS304; the builder merges them by term (DS304's
   text wins as the first course alphabetically). Other glossary links of these chapters
   point to terms defined in other courses' glossary.json files (checked to exist at build time).
