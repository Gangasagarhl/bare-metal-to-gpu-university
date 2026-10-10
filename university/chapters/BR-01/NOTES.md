# BR-01 — From C++ to CUDA C++: the same language, a second place to run (author notes)

Bridge chapter, level L2, placed between SP203 and CU201 (prerequisite of F6-01, whose front
matter already lists BR-01). Fragment: `university/chapters/BR-01/BR-01.html` (front matter
`id: BR-01`, `course: BR`). Lab: `university/labs/BR-01`. Glossary proposals:
`university/chapters/BR-01/glossary.json` (6 new terms; 23 existing terms linked, not redefined).

`university/labs/run_lab.sh university/labs/BR-01` exits with status 0 (re-run after the last
change, 2026-10-10). `python3 university/build/build.py` reports no PROBLEM line that mentions
BR-01 and no broken link that targets an anchor used in BR-01 (the remaining PROBLEM lines
belong to other chapters, for example BR-03's source anchors).

## Toolchain (as recorded in the logs)

- nvcc: `Cuda compilation tools, release 12.0, V12.0.140` (Ubuntu package nvidia-cuda-dev
  12.0.146~12.0.1-4build4); default device target chosen by this nvcc: sm_52 (visible in the
  dryrun and cuobjdump output); cuobjdump from the same toolkit; GNU nm.
- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0 with
  `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined` for the CPU listings.
- nvcc listings: `nvcc -std=c++17 -O2 -lineinfo -Werror all-warnings <file>.cu` (run_lab.sh).

## Listings and runs

**No GPU exists in the build container.** Every `.cu` program was really compiled warning-free
and really run. Programs with no runtime call (Listing 2, the step-2 copy of Listing 1) ran to
completion on the CPU and passed their own check. Programs that call the runtime stop at their
first call with the runtime's own `cudaErrorNoDevice`; the chapter shows that output and an
"untested on hardware" box with the expected GPU output. run_lab.sh writes the generic
"untested on hardware" line into every `.cu` log, including the CPU-only Listing 2.

| Run (`.log`) | What | Result | Status |
|---|---|---|---|
| step1_cpu | Listing 1, CPU program (g++) | exit 0 | pass |
| step2_nvcc | Listing 1 copied unchanged to .cu, built with nvcc (run.sh) | exit 0, same output | pass (CPU only) |
| step3_qualifiers | Listing 2, specifiers + `__CUDA_ARCH__` probe | exit 0, prints "host pass" | pass (CPU only) |
| step4_memory | Listing 4, DeviceBuffer round trip (uses Listing 3 header) | exit 1, cudaErrorNoDevice | expected; untested on hardware |
| step5_kernel | Listing 5, finished port | exit 1, cudaErrorNoDevice | expected; untested on hardware |
| device_library | Listing 6, device-code probe | compile failed as expected (std::max, constexpr helper) | expected-fail |
| device_library_check | Listing 6 minus the refused lines; and with --expt-relaxed-constexpr (run.sh) | exit 0 | pass (compile only) |
| grid_model | Listing 7, CPU launch model; unchecked run caught by ASan | exit 1 after ASan report | expected (teaching model) |
| stream_model | Listing 8, CPU model of an asynchronous launch | exit 0 | pass (teaching model) |
| break3_unchecked | Listing 9 / lab break 3, every check removed | exit 0, all zeros | expected; untested on hardware |
| worked | Listing 10, Worked example arithmetic | exit 0 | pass |
| break1_no_qualifier | lab break 1 (specifiers removed from add) | compile failed as expected | expected-fail |
| break2_exception | lab break 2 (throw in kernel) | compile failed as expected | expected-fail |
| break_host_pointer | trap 3, host pointers passed to the kernel | compiles warning-free; exit 1 cudaErrorNoDevice | untested on hardware |
| diffs | diffs of the three changed files against Listing 5 (run.sh) | exit 1 (diff: files differ) | pass (expected status) |
| split | `nvcc --dryrun` on Listing 5 filtered by split_filter.py (run.sh) | exit 0 | pass (compile-only listing) |
| fatbin | cuobjdump --list-elf --list-ptx and nm -C on built Listing 5 (run.sh) | exit 0 | pass (inspected, not run) |
| nvcc_help | nvcc --help excerpts (--dryrun, --expt-relaxed-constexpr, --x) | exit 0 | pass |
| forensic_port, forensic_port2, forensic_port3 | forensic evidence: three builds of Kwame's port | compile failed as expected (1, 1 and 10 errors) | expected-fail |

`split_filter.py` (a small Python filter used by run.sh) and `run.sh` are lab tooling, not
chapter listings. All generated binaries and temporary files are deleted by run.sh.

## Unverified boxes in the chapter (owner approval needed before publication, AH-19)

1. Layer 2: the exact list of C++ features and standard-library facilities supported in device
   code (Programming Guide, C++ language support section), and whether NVIDIA's documentation
   calls `__host__`/`__device__`/`__global__` "execution space specifiers" or "qualifiers" (the
   bridge card says "qualifiers"; the chapter uses "specifier" and flags it).
2. Code walk-through: Listings 4 and 5 untested on hardware; expected GPU outputs stated, not observed.
3. Trap 1: what an early host read of managed memory really returns on a GPU; the role of
   `cudaDevAttrConcurrentManagedAccess` (named in the header, rules not read).
4. Trap 3: what happens at run time when a kernel receives a host pointer (cudaErrorIllegalAddress
   is described in the header; whether and where this program gets it, or whether a system with
   special memory support can access host memory, was not observable).

## Claims that rest on documents not opened in this build (gate G1 open)

- D1 CUDA C++ Programming Guide: kernels, thread hierarchy, specifiers, execution configuration,
  device-code restrictions, unified memory. Every D1 tag is pending verification.
- D3 CUDA Compiler Driver NVCC: the compilation trajectory. The chapter's description of the two
  "roads" is drawn from this build's real `--dryrun` output; the interpretation that step 7's
  `-D__CUDA_ARCH__` has no effect because its input was preprocessed in step 1 is ours (stated
  as such in Layer 3), consistent with Listing 2 printing "host pass".
- D3/R8: the reading of the `__device_stub__` symbol as host code that hands the launch to the
  runtime is an interpretation from names (stated as such).
- D4 PMPP and D5 "A Tour of C++": conceptual claims only (two memories on a discrete card;
  RAII, destructors must not throw, templates).
- What nvcc did or refused is fact for CUDA 12.0 in this container (R-tags); whether a facility
  that compiled (std::sqrt, printf in device code) is officially supported is D1's question and is
  said so in the chapter.

## Decisions for the owner

1. **Length.** The prose is long for L2 (about 8,900 words counted with the jargon box, sources
   and answers; the brief asks for about 3,000). The bridge card requires the "Carries over",
   "Changes" and "Traps" lists each in depth with a real run per trap, plus a five-step lab and
   three breaks; I kept all of it. If the owner wants the lower end, the candidates to move out
   are Layer 3's binary inspection (fatbin) and the compile-error table.
2. **Error-check style.** Listing 3 throws a `std::runtime_error` from `cudaCheck` and owns device
   memory with an RAII `DeviceBuffer<T>`, to show that exceptions and RAII carry over on the host.
   CU201 (F6-05) prints and exits instead. The chapter says both styles are valid; the Dean may
   want one house style.
3. **New analogy mappings (proposals for the registry, guide 8.1; not registered yet):**
   - execution-space specifier = a label on each recipe: "chef only", "hall only", or "both";
   - nvcc = a translator who splits the recipe book, translates the hall's cards, hands the
     chef's pages to the chef's usual translator and binds everything into one book (extends the
     F1/F2 "compiler = translator" mapping);
   - last error = a note pinned at the pass between kitchen and hall that nobody reads to you;
   - device pointer and host pointer = shelf numbers in two different pantries; the note does not
     say which pantry;
   - managed memory = a shared pantry whose shelves a porter moves between buildings on demand.
   Mappings used as registered: CPU = head chef, GPU = hall of helpers, kernel = recipe card,
   order slip handed to the hall = launch (as F6-01 already uses), several cooks in one kitchen
   = threads (SP203), RAII = borrowed tool returned automatically.
4. **Folder.** The guide's 7.1 puts bridges in `bridges/`; the build reads fragments from
   `chapters/<folder>/`, so this one lives in `chapters/BR-01/` as the task asked.
5. **Teaching models.** Listings 7 and 8 are the university's own CPU models (a launch as two
   loops with AddressSanitizer; an asynchronous launch as a queue run at synchronize). Both are
   labelled "not CUDA" in the code and the text.
6. **Hardware.** A GPU run of Listings 4, 5, 9 and break_host_pointer (and Compute Sanitizer on
   Listing 5 and on a no-bounds-check variant) is needed to close the untested-on-hardware notes.

## Cross-links used

F2-02, F2-05, F2-10, F2-11, F2-14, F2-16, F2-19, F2-25, F2-27, F2-29, F2-34, F2-35, F2-41, F2-42,
F1-55, F1-56, F1-62, F3-54, F7-06, F6-01, F6-04, F6-05, F6-07, F6-09, F6-28, F6-31, F6-32, F6-34,
SP102, SP201, SP202, SP203, CU201, MA102, MA302, #bridges; glossary anchors checked by the build.
