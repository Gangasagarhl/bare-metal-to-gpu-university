# Brief given to the chapter-author agents (batches 2 onward: L1–L5 courses)

Everything in `AUTHOR_BRIEF.md` (batch 1) still applies: read it first. This file adds what
changes for the higher levels. Where they differ, this file wins.

## Inputs

1. `UNIVERSITY_AUTHORING_GUIDE.html`: "Honesty first", sections 2, 3, 6, 7, 8 (your
   faculty's analogy world and the rules), 9, 10, 11, 13, 14, plus **your course card** in
   section 5, the **bridge chapters** of section 6.2 that touch your course, and the source
   registry rows of section 4 for your faculty.
2. `SYSTEMS_CURRICULUM.html` (repo root): the course card's "Maps to" names curriculum
   milestones (A1–A5, B1–B18, C1–C15, D1–D10, E1–E12, F1–F8, H1–H5, FS, U, V, M, G, P...).
   **Read those milestone blocks.** Labs reuse the milestone's goal and its acceptance tests
   *verbatim* (guide 11.1 row 6). The curriculum is a map, not a source (AH-4): it may be
   quoted for milestone ids, acceptance tests and reading lists, but technical claims are
   tagged to the documents it names (title only, pending verification).
3. `university/chapters/FRAGMENT_FORMAT.md`, `university/labs/run_lab.sh`,
   `university/labs/TOOLCHAIN_VERSIONS.txt`.
4. Already written chapters of earlier courses in `university/chapters/` (read a couple
   in your area to keep terms, analogies and cross-links consistent).

## What the build container really has (use it; record versions from the tools' output)

- Host C++: g++ 13.3, clang++ 18, cmake, gdb 15, valgrind, strace/ltrace, perf (may not
  work in the container; if it fails, say so), nasm, ld.lld, binutils, elfutils.
- Emulators: `qemu-system-x86_64`, `qemu-system-aarch64`, `qemu-system-riscv64` (QEMU 8.2.2)
  with OVMF in `/usr/share/OVMF/` and `/usr/share/ovmf/`, AArch64 UEFI firmware
  (qemu-efi-aarch64); cross compilers `aarch64-linux-gnu-g++`, `riscv64-linux-gnu-g++`;
  mtools, gdisk, dosfstools. **Firmware, boot and kernel labs can really boot in QEMU**
  (use `-nographic`/`-serial stdio`, `-no-reboot`, a `timeout`, and the isa-debug-exit
  device as the curriculum describes). Keep code small enough to be a chapter listing;
  a lab may have several files.
- GPU: `nvcc` (CUDA 12.0) and `cuobjdump`, `nvdisasm`, `compute-sanitizer`; `hipcc`
  (HIP 5.7, offload target gfx90a). **There is no GPU**: CUDA/HIP listings are compiled for
  real and the run shows the runtime's own "no device" error; mark them *untested on
  hardware* (AH-26). PTX/SASS (`cuobjdump -ptx/-sass`) and AMD ISA (`hipcc --save-temps`)
  can be produced for real and read in chapters: do that, it is genuine evidence.
  Never write a measured GPU time or bandwidth: there is none.
- HDL: iverilog 12, verilator 5.020, yosys 0.33 (gate-level and sequential-logic labs can
  be simulated for real).
- MPI: Open MPI 4.1.6 (`mpirun --oversubscribe` on CPU processes; no GPUs, no RDMA).
- tshark (packets can be captured only on loopback inside the container, if permitted;
  otherwise say so).
- Python 3 with numpy (for plotting-free analysis of logged data).
- **Not available**: internet, GPUs, boards/microcontrollers, robot or drone kits, ROS 2,
  PX4, ArduPilot, Gazebo, Slurm, Kubernetes, RDMA NICs, a second machine. For these, write
  the university's own small C++/Python simulators where that teaches the idea honestly,
  show their real output, and mark the real-tool lab steps *untested in this build* with
  exactly what would be needed. Never write a command line, parameter name, message name
  or API of these tools from memory as if verified: put such details in an
  **unverified box** naming the documentation to check (AH-17, AH-18).

## Running anything other than .cpp/.cu/.hip

`run_lab.sh` compiles every `*.cpp`, `*.cu`, `*.hip` and then runs an executable `run.sh`
in the lab folder if present. Use `run.sh` for QEMU boots, gdb batch sessions, HDL
simulations, MPI runs, multi-file builds, etc. It must write, for each step `<name>`,
`<name>.out` (the real output) and `<name>.log` with the same fields as run_lab.sh writes
(`listing:`, `toolchain:` version line, `command:`, `date:`, `machine:`, then
`exit code: N` or `result: …`; add `hardware: untested on hardware …` where relevant).
Reference them in the chapter with `<pre class="output" data-run="<ID>/<name>">`.
Listings in other languages (`.S`, `.ld`, `.v`, `.sh`, `.py`, `Makefile`, `CMakeLists.txt`)
are shown with `<pre class="listing" data-src="<ID>/<file>">`. Everything must pass
when `run_lab.sh university/labs/<ID>` is executed from the repository root; check that.
Keep generated binaries/disk images out of the folder (delete them at the end of run.sh)
so the repository stays small; keep sources, inputs, `.out`, `.log`.

## Length and depth (guide 2.1)

Aim for the lower end of each range so the whole course gets written: L1 about 2,000
words, L2 about 3,000, L3 about 4,000, L4 about 5,000, L5 about 3,000 plus project
material (prose, excluding code and tables). Depth and code share as in the guide's
table; L3+ chapters cite specification *sections by title* (pending verification) and
analogies shrink to one short paragraph, but the story hook and "Where the analogy
breaks" remain.

## Faculty analogy worlds (guide 8.1)

F1, F2: the restaurant building and its kitchen. F3, F4: the school (and its visitors).
F5: friends in different towns. F6, F7, F8: the great kitchen hall. F9: riding a bicycle;
a body with senses and muscles. F10: carrying a tray of drinks while walking. F11: a castle
with gates and guards. F12: building a house as a team. Use only registered mappings; if
you need a new one, add it to your NOTES.md as a proposal instead of inventing it silently.

## Deliverables (per course)

Same as batch 1: `university/chapters/<COURSE>/<ID>.html` for every chapter,
`university/labs/<ID>/...`, `university/chapters/<COURSE>/glossary.json`,
`university/chapters/<COURSE>/NOTES.md`. Validate HTML balance and id prefixes; no URLs;
no scripts. Do not edit outside your course's folders; do not commit or push; do not call
any `mcp__hearthbot__` tool.
