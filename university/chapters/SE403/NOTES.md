# SE403 — Reading unfamiliar codebases: author's notes

Chapters: F12-20 Maps before details · F12-21 Tracing a request through Linux, PX4 and ROS 2 · F12-22 Working with legacy code.
Labs: `university/labs/F12-20`, `university/labs/F12-21`, `university/labs/F12-22`. Each passes `run_lab.sh` (exit 0) in the build container
(Linux 6.18 x86_64, running as root, g++ 13.3.0, Python 3.13.16, strace 6.8, QEMU 8.2.2 with SeaBIOS 1.16.3). There was no internet.

## Listings run

| Chapter | Run | Status |
|---|---|---|
| F12-20 | codemap (C++ map of the Python 3.13.16 standard library) | pass |
| F12-20 | server_log, probe, logline_search, hub_codecs (run.sh) | pass |
| F12-21 | ifstream_read; ifstream_strace (strace; LeakSanitizer off for that run only) | pass |
| F12-21 | read_stack (kernel stack samples of read() with O_DIRECT, from /proc/self/task/<tid>/stack; needs root) | pass; the output varies per run |
| F12-21 | nvme_qemu (QEMU emulated NVMe controller; SeaBIOS reads the boot sector; trace events show a Read command) | pass, exit 3 expected; **emulated only, untested on a real NVMe drive** |
| F12-21 | uorb_trace, uorb_polled (discrete-event model of a uORB/DDS path) | pass; a **model**, not PX4 or ROS 2 |
| F12-21 | tools_check (perf, tracefs, bpftrace, trace-cmd, blktrace missing) | pass (records absence) |
| F12-22 | pin_attempt (output varies per run), characterize, refactor_check, cleanup_check | pass |
| F12-22 | seam_diff (diff -u) | pass; diff status 1 expected |

Not run in this build: F12-21 lab steps 5–6 (reading the Linux source tree, PX4 source, ROS 2 on a real system); the course
mini-projects and the course project (learner work).

## Unverified boxes (need the Source Researcher)

- F12-20: names and conventions of entry points in other ecosystems (written from memory); D1 Feathers not opened.
- F12-21: Linux source file paths and function names beyond those seen in /proc/kallsyms and the stack samples
  (fs/read_write.c etc.); NVMe command names beyond those printed by QEMU's trace; PX4 topic and module names
  (sensor_accel, sensors, EKF2, uXRCE-DDS bridge) and ROS 2 layer names; lab steps 5–6 untested.
- F12-22: Feathers terms (characterization test, seam kinds object/link/preprocessor, sprout/wrap method, edit and pray
  vs cover and modify) recalled from memory; "golden master" as a common name. The printf result 2.25 → "2.2" is observed,
  not explained.
- All "D" sources outside this project (Feathers, Linux docs, man-pages, NVMe spec, QEMU docs, PX4 guide, ROS 2 docs) are
  title only: dossier gate G1 open.

## Proposed analogy mappings (F12 — building a house as a team)

- F12-20: codebase map = the floor plan you sketch on a first walk through an old house; entry point = a door; hub module =
  the main pipe every room is connected to; log line = a mark on a wall — find the tool that made it.
- F12-21: tracing a request = following one water pipe from the tap back to the street, joint by joint; hop = a joint where
  the pipe changes size, material or owner; buffer or queue at a hop = a tank where water waits.
- F12-22: legacy code = an old house people still live in; characterization test = photographing and testing every room,
  switch and tap before touching anything; golden master = the photo album; seam = a junction box where one cable can be
  disconnected to test a room alone; sprout = a new room built beside the old one. Uses the registered row
  "code review = a second carpenter checks your joints".

## Decisions for the owner

1. **Mapped codebase.** F12-20 maps the Python standard library (present in the container) instead of Linux/PX4/ROS 2,
   which were not available. The forensic "where does this log line come from?" uses http.server's real log line.
2. **NVMe evidence.** The "read() to an NVMe command" lab is split: the Linux half uses kernel stack samples on the
   container's virtio-blk disk (no NVMe there); the NVMe half uses QEMU's emulated controller with SeaBIOS as the host,
   not Linux. A full Linux-guest-on-emulated-NVMe run (needs a guest kernel image) or real hardware would close the gap.
3. **uORB lab as a model.** PX4 and ROS 2 could not be installed; the uORB path is a discrete-event model clearly labelled
   as such. Please decide whether to require a PX4 SITL run in a later build.
4. **Root needed.** read_stack reads /proc/self/task/<tid>/stack, which needs root (the build ran as root).
5. **Outputs that vary per run.** read_stack, pin_attempt and the server_log dates change each run; prose avoids depending
   on exact values from them.
6. **Repository file.** `labs/F12-21/sample.txt` (22,000 bytes, generated) is committed as lab input.
7. **Course-level items.** The timed code-reading exam (exam P) and the course project are described across the three
   chapters' mini-projects (project parts 1–3); no separate exam paper was written.
8. **Forensic code in F12-22** (`status_cleanup.hpp`) was written for the lab; the answer key says so.
