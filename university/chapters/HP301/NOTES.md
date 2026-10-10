# HP301 HIP I: porting from CUDA — author and lab-engineer notes

Chapters F7-01 to F7-07, labs in `university/labs/F7-01` … `F7-07`, glossary in `glossary.json` (38 entries).
Build container: Ubuntu 24.04.5 LTS, kernel 6.18.44-fc-v114; HIP 5.7.31921 from the Ubuntu "noble" universe
packages (hipcc 5.7.1-3, libamdhip64 5.7.1-3, libhsa-runtime64 5.7.1-2build1, rocminfo 5.7.1-3build1,
libamd-comgr2 and rocm-device-libs-17 with 6.0 snapshot versions), clang 17.0.6, nvcc 12.0 (V12.0.140),
CMake 3.28.3, g++ 13.3.0, Python 3.13. **No GPU, no ROCm kernel driver** (`/dev/kfd` absent).
**hipify-perl and hipify-clang were not installed**; the course uses its own teaching translator
`toyhipify.py`, labelled as such everywhere.

All seven labs pass `university/labs/run_lab.sh university/labs/F7-0N` (re-run at the end of this build).

## Conventions used in every chapter

- Rn = a run in this build (file `<lab>/<name>.log`); Hn = an installed header or package file read in
  this build, with line numbers (HIP 5.7.1 headers); Dn = a document, title only, not opened (gate G1 open).
- Every HIP/CUDA program was compiled for real; every run stopped at the runtime's "no device" error and
  is marked *untested on hardware*. Assembly (AMD ISA via `--save-temps`), PTX and SASS are real output.
- Common NVIDIA-back-end command in this container: `HIP_PLATFORM=nvidia CUDA_PATH=/usr hipcc -std=c++17 -x cu …`
  (`CUDA_PATH=/usr` because CUDA is installed under `/usr`; hipcc first failed with
  `/usr/local/cuda/bin/nvcc: not found`; nvcc refuses `.hip` without `-x cu`).

## Per chapter

### F7-01 Same recipes, a second kitchen building: HIP and ROCm
- Listings run: `hello_hip.hip` (AMD build, exit 1, untested on hardware), `run.sh` steps `tools`,
  `bundle` (roc-obj-ls), `hello_nvidia` (NVIDIA back end, exit 1, untested on hardware).
- Unverified boxes: expansion of the name HIP; list of ROCm components; kernel driver in a full install;
  need for `CUDA_PATH=/usr`; expected output on a GPU.
- R7 (hipcc's NVIDIA command line) now points to `F7-07/nvidia_driver.log` (it was a NOTES-only record before).

### F7-02 Installing ROCm and checking compatibility
- Listings run: `devquery.hip` (AMD, exit 1), `run.sh` steps `system` (OS, kernel, package versions),
  `check` (`check_rocm.sh`, exit 1 with three real FAIL lines: no `/dev/kfd`, no render nodes,
  "ROCk module is NOT loaded"), `devquery_nvidia` (exit 1), `bundle_targets` (one and two targets).
- Unverified boxes: no supported GPU/OS/kernel named (matrix not opened); `/dev/kfd`, render nodes and
  `video`/`render` group membership; install commands deliberately omitted; `/opt/rocm` layout of AMD's
  packages; HIP_VERSION digit layout (inferred from 5.7.31921 = 50731921); NVIDIA driver version 13000
  beside toolkit 12.0 not investigated; HSA queue/dispatch description; meaning of `gfx000` printed by
  `rocm_agent_enumerator`; device-report lines never printed.
- Worked example uses a constructed checklist output (built from the script's real message strings; stated).

### F7-03 The HIP API mapped to the CUDA runtime
- Listings run: `hipmap.py` (reads the NVIDIA back-end header: 665 mapped names; 27 of 29 CU201 names
  differ only by prefix; `hipHostMalloc`/`cudaHostAlloc` and `hipHostFree`/`cudaFreeHost` differ),
  `api_tour.hip` (AMD exit 1 at `hipHostMalloc` with `hipErrorInvalidDevice`; NVIDIA exit 1 with
  `cudaErrorNoDevice`), `unchecked.hip` (**new in this build**: exit 0 printing "sum of 256 ones = 0";
  step `unchecked_warnings` shows `[[nodiscard]]` warnings on both back ends), `leftover.hip`
  (**expected-fail** on AMD: `unknown type name 'cudaStream_t'`; NVIDIA build succeeds).
- Answers to Check yourself 5 and 6 were run by the author (not lab files):
  `hipmap.py … hipMallocHost hipHostMalloc` printed `function hipMallocHost cudaMallocHost line 1514` and
  `function hipHostMalloc cudaHostAlloc line 1528 <- name differs`; `unchecked.hip` with
  `check(hipGetLastError(), "launch")` printed `launch failed: hipErrorNoDevice (no ROCm-capable device is detected)`
  (AMD, exit 1) and `launch failed: cudaErrorNoDevice (…)` (NVIDIA back end, exit 1).
- Unverified boxes: behavioural equivalence of mapped calls (default stream, flags, managed memory);
  665 is the script's count, not official; DMA/pinned/event description.

### F7-04 hipify-perl and hipify-clang
- Listings run: `toyhipify.py` on `saxpy.cu` (12 of 61 lines changed, 0 untranslated) and `warp_sum.cu`
  (12 of 45, `__shfl_down_sync` untranslated, 2 lane warnings); `gen/saxpy.hip` on both back ends (exit 1,
  untested on hardware); `gen/warp_sum.hip` AMD build **fails as expected** (`use of undeclared identifier
  '__shfl_down_sync'; did you mean '__shfl_down'?`, run.sh step exit 1, recorded), NVIDIA build succeeds;
  `saxpy.cu`, `warp_sum.cu` with nvcc (exit 1).
- Check yourself 6 run by the author: renaming `mismatches` to `cudaStatus` gave `lines changed: 12 of 61`,
  `CUDA-looking identifiers NOT translated: 5`; the result built for gfx90a.
- Unverified boxes: everything about the real HIPIFY tools (installation, options, output, statistics,
  launch-syntax rewriting, treatment of `_sync` intrinsics); `_sync` warp functions in later HIP versions.
- The forensic answer key's arithmetic for `__shfl_down` at the wavefront edge is reasoned from the header,
  not run.

### F7-05 Vector add in HIP, line by line
- Listings run: `vec_add.hip` (AMD exit 1, NVIDIA back end exit 1), `vec_add_b.hip` (forensic build B,
  bounds check removed deliberately, AMD exit 1), `isa_of.sh` steps `isa` and `isa_b` (real gfx90a ISA:
  A has 13 SGPRs / 8 VGPRs with `v_cmp_gt_i32` + `s_and_saveexec_b64` + `s_cbranch_execz`; B has 16 / 5 and
  no compare). PTX/SASS comparison reuses `F7-07/vec_add_ptx`.
- Check yourself 5 run by the author: `./isa_of.sh vec_add.hip gfx1030` gave `.wavefront_size: 32`,
  `.sgpr_count: 11`, `.vgpr_count: 6`, `s_mov_b32 s0, exec_lo` + `v_cmpx_gt_i32_e64 s1, v0` +
  `s_cbranch_execz`, index via `v_mad_u64_u32`.
- Unverified boxes: dispatch-packet pointer in `s[4:5]`, kernarg pointer in `s[6:7]`, workgroup id in `s8`,
  work-item id in `v0` (consistent with the code, not read in D5); occupancy arithmetic left to HP302;
  expected GPU output.

### F7-06 Building with hipcc and CMake
- Listings run (`run.sh`): `hipcc_driver` (HIPCC_VERBOSE command), `hipcc_default` (no `--offload-arch`
  → bundle has **gfx906** only), `cmake_configure`/`cmake_build` (find_package(hip), GPU_TARGETS
  gfx90a;gfx1030, compile and link lines, bundle), `cmake_run` (exit 1), `cmake_notargets` (empty
  GPU_TARGETS → gfx906, forensic evidence), `cmake_hiplang` (CMake 3.28 HIP language **fails** in this
  container: no `hip-lang-config.cmake` under /usr/lib/cmake; the step records CMake's message),
  **new:** `cmake_nvidia` and `cmake_nvidia_run` (same `proj/` with `-DHIP_BACKEND=NVIDIA`: nvcc compiles the
  `.hip` file as CUDA; exit 1 at run, untested on hardware).
- `proj/CMakeLists.txt` was extended in this build to build both back ends from one tree (`HIP_BACKEND`).
  An earlier draft defined `__HIP_PLATFORM_NVIDIA__` by hand; `hip_common.h` lines 44–52 define it when
  nvcc compiles, so the line was removed and the build re-run (identical result).
- Unverified boxes: meaning of hipcc's two `-mllvm` options; which installations provide
  `hip-lang-config.cmake`; NVIDIA-platform CMake recipe is the author's own (verified to build only);
  error message on a GPU missing its target; "generic" targets in newer ROCm.

### F7-07 Running HIP on NVIDIA through the CUDA back end
- Listings run: `wave_sum.hip`, `wave_sum_fixed.hip`, `warp_size.hip` (AMD builds exit 1; NVIDIA builds
  exit 1 — all untested on hardware), `lanes_sim.cpp` (the university's CPU lane model, exit 0; it copies
  HIP 5.7's `__shfl_xor` source-lane rule from the AMD header), `run.sh` steps `header_lines`,
  `warp_size_isa` (64 for gfx90a, 32 for gfx1030), `warp_size_ptx` (`WARP_SZ`), `shuffle_count`
  (5/5/5 ported; 6/5/1 fixed for gfx90a/gfx1030/sm_80), **new:** `nvidia_driver` (hipcc's nvcc command),
  `vec_add_ptx` (PTX + SASS of F7-05's vecAdd), `nvidia_fatbin` (HIP-on-NVIDIA and plain nvcc fat
  binaries have the same entries; HIP build links `libcudart.so.12` dynamically, plain nvcc lists none).
- The course forensic "Correct on NVIDIA, wrong on AMD" uses the lane model's output as the per-lane
  evidence (no GPU); this is stated in the scenario and the key.
- Unverified boxes: GPU outputs (predicted from the model); `_sync`/64-bit-mask functions and the nature of
  `warpSize` in newer HIP; SHFL / ds_bpermute hardware descriptions.
- Observation not explained: `cuobjdump --list-elf` shows two cubins per architecture for both HIP-on-NVIDIA
  and plain nvcc executables in this container (stated in the lab's troubleshooting, not investigated).

## Decisions for the owner

1. **HIPIFY tools.** The real hipify-perl / hipify-clang were unavailable; F7-04 teaches with
   `toyhipify.py` and puts every HIPIFY detail in unverified boxes. When a machine with ROCm's HIPIFY is
   available, add a lab step that runs both tools on `saxpy.cu` and `warp_sum.cu` and compare with the toy.
2. **Hardware for the course.** Every GPU result is untested. The course needs at least one wave64 AMD GPU
   (CDNA, from the ROCm matrix) and one NVIDIA GPU, or cloud time, to run the E9 acceptance tests and to
   confirm the lane-model predictions of F7-07 (ported total 528 on gfx90a).
3. **ROCm packaging.** The container's distribution packages lack `hip-lang-config.cmake`, so CMake's HIP
   language could not be taught by running it. Decide whether the course standardises on AMD's own ROCm
   packages (then re-run F7-06 `cmake_hiplang`) or keeps `find_package(hip)` as the main path.
4. **HIP version.** All header line numbers and behaviours are HIP 5.7.1 (Ubuntu). A dossier pass should
   pick the ROCm version the university will use and re-run every lab (hipmap counts, warp functions,
   `[[nodiscard]]`, `warpSize` declaration can change).
5. **Glossary overlap.** HP301 defines "Wavefront" and "warpSize (HIP)"; HW301 has "Warp / wavefront" and
   HP302 links `#gl-warpsize-hip` and other terms. If HP302 later defines the same term strings, the builder
   merges them; check wording consistency then.
6. **Analogy registry.** Story characters (Amara, Ravi, Noor, Viktor, Tomás, Kwame, Sipho, Ines) and the
   mappings "second kitchen building = ROCm/AMD", "shared cookbook language = HIP", "prep chef = hipcc",
   "plan of work = CMake", "maintenance list = compatibility matrix", "translator = HIPIFY" extend the F7
   great-kitchen-hall world (BR-02 names "a second kitchen building"). Proposed for the registry (guide 8.1).

## Exams (course card: Q; M; F; P)

Not written in this batch (no exam files were requested in the chapter task). Suggested P exam material is
ready in the labs: port `F7-04/warp_sum.cu` and justify every change (rename, intrinsic, loop start,
leader rule, mask width), with the AMD compile, the shuffle counts and the lane model as evidence.
