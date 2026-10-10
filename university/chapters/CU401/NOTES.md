# CU401 CUDA V: libraries, multi-GPU and fused kernels — author's notes

Chapters F6-35 to F6-40 (L4), labs in `university/labs/F6-35` … `F6-40`, glossary in
`glossary.json` (37 four-part entries; sources "pending verification" unless they are
installed headers read in this build).

This course was written in two runs. The first run (interrupted by a container restart)
wrote all six lab folders and the chapter F6-38. The second run reviewed and kept those,
wrote F6-35, F6-36, F6-37, F6-39, F6-40, the glossary and these notes, added a
`header_quotes` step to the F6-35 and F6-36 labs, and re-ran every lab.

Build environment of every run: g++ 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1), nvcc /
cuobjdump from CUDA 12.0 (V12.0.140), cuBLAS 12.0.2.224, Thrust and CUB 2.0.1, Python
3.13.16 with numpy 2.5.3. **No GPU, no cuDNN, no Triton, no PyTorch.** Every `.cu`
listing is built for real and its run records the runtime's (or cuBLAS's) own error,
exit code 1, `hardware: untested on hardware`. PTX/SASS/ptxas reports and header
excerpts are produced by each lab's `run.sh` (compiled only / read only).

All six labs were re-run at the end with `university/labs/run_lab.sh university/labs/F6-3x`
and `F6-40`: every run returned 0. `python3 university/build/build.py` reports no PROBLEM
line for these chapters (the remaining PROBLEM lines belong to other courses).

## Global facts for the owner

- **No GPU number anywhere.** No time, bandwidth, speed-up, occupancy or "% of library" was
  measured. All such values are learner measurements ("your X"). The only GPU-like numbers
  are those of the university's invented TG-1 / TN-4 models, always labelled invented.
- **No document was opened.** CUDA C++ Programming Guide, PTX ISA, CUDA Binary Utilities,
  cuBLAS / CUB / Thrust / cuDNN / CUTLASS documentation, Triton documentation, PCIe
  specification, NVIDIA/AMD interconnect documents, PMPP, Stevens and Rago (APUE), and the
  papers (BLAS: Lawson et al., Dongarra et al.; Merrill and Garland; Milakov and Gimelshein;
  Welford; Chan, Golub and LeVeque; Ba, Kiros and Hinton; Vaswani et al.; Dao et al.
  FlashAttention and FlashAttention-2; Tillet, Kung and Cox) are cited **title only**
  (dossier gate G1 open).
- **Tier-1 evidence really read:** installed CUDA 12.0 headers (`cuda_runtime_api.h`,
  `driver_types.h`, `cooperative_groups.h` and `details/info.h`, `details/sync.h`,
  `crt/device_functions.h`, `sm_30_intrinsics.h`), cuBLAS 12.0.2 headers (`cublas_v2.h`,
  `cublas_api.h`), CUB 2.0.1 headers (`device_reduce.cuh`, `device_scan.cuh`,
  `block_reduce.cuh`), Thrust 2.0.1 `version.h`. Quotes in F6-35, F6-36 and F6-37 are
  reproduced by lab steps (`header_quotes`, `p2p_api`); F6-38 and F6-39 quote
  `__expf` / `__shfl_xor_sync` from the headers directly (not yet through a lab step).
- **Invented teaching hardware:** TG-1 (the course's teaching GPU, glossary entry from
  HW301) is reused in F6-36 (4 SMs, residency model) and F6-37 (host link 10 µs,
  16 GB/s, as in F6-29). **TN-4** (four TG-1 behind two invented PCIe switches) is new
  in F6-37; its link numbers are invented for arithmetic. Decision for the owner:
  register TN-4 next to TG-1 in the glossary/analogy registry, or rename.

## Per chapter

### F6-35 Standing on giants: cuBLAS, CUB, Thrust, cuDNN
- Listings run: `colmajor.cpp` (pass), `thrust_host.cpp` (pass, Thrust host back end),
  `forensic.cpp` (pass), `thrust_device.cu`, `cub_sum.cu`, `cublas/sgemm_cublas.cu`
  (built; untested on hardware, exit 1), `thrust_v1` (first version without the
  `bad_alloc` handler: aborts with exit 134, kept as evidence), `cub_resources`
  (ptxas, compiled only), `lib_versions` (dpkg), `header_quotes` (read only).
- Real finding of this build: a failed `device_vector` allocation arrived as
  `thrust::system::detail::bad_alloc` (derived from `std::bad_alloc`), not as
  `thrust::system_error`; the listing catches both.
- `cublas/sgemm_cublas.cu` lives in a subfolder so that `run_lab.sh` (which has no
  `-lcublas`) does not build it; `run.sh` builds it with `-lcublas`.
- Unverified boxes: (1) cuDNN API (not installed; no names written); (2) cuBLAS batched
  routines and cuBLASLt epilogue names; (3) Thrust stream selection through an execution
  policy (`thrust::cuda::par.on`); (4) untested on hardware and "cuBLAS selects among many
  kernels" (PMPP / CUTLASS, to confirm in a profiler).
- Analogy proposal (needs registration by the Dean): **library = a specialist station of
  the hall with its own order form** (calling convention). Not in the registered F6 table.

### F6-36 Cooperative groups
- Listings run: `cg_sum.cu` (built; untested on hardware), `residency.cpp` (pass),
  `tiles.cpp` (pass), `forensic` (Listing 2 on the forensic input, pass), `sass_sync`,
  `resources` (compiled only), `header_quotes` (read only).
- The grid-barrier algorithm (release atomic add, acquire polling of the top bit) and
  the trap on invalid `grid.sync()` are quoted from the installed headers and matched to
  SASS instruction counts.
- Unverified boxes: (1) roles of `MEMBAR.ALL.GPU` / `CCTL.IVALL` in SASS; (2) untested
  on hardware, and whether any build flag (e.g. relocatable device code) is needed for
  grid sync on other toolkit versions (none was needed to compile and link here).
- Glossary: "Cooperative launch" already exists in CU301's glossary (F6-12) with a
  consistent definition; not duplicated here — the Integrator may add F6-36 to its
  "chapters" list. "Resident" links to the HW301 entry.
- Lab safety note: a deliberate hang can freeze a display GPU; step 6 is optional and
  only on a non-display GPU.

### F6-37 Multi-GPU on one node: peer access and IPC (milestone F1)
- Listings run: `p2p_matrix.cu`, `ipc_pair.cu` (built; untested on hardware; the IPC
  program's parent/child error path ran for real), `topo_model.cpp` (pass), `forensic`
  (model, pass), `p2p_api` (header declarations and enum values, read only).
- Unverified boxes: (1) "fork before the first CUDA call" rule (not in the headers read);
  (2) the nvidia-smi topology command and legend, and statements about platforms that
  disable or slow peer access; (3) how switches/root complexes forward peer transactions,
  untested on hardware, all Figure 1 numbers invented.
- The forensic answer key simplifies which device's peer enable a `cudaMemcpyPeerAsync`
  depends on; flagged in the key itself for checking against the Programming Guide.
- Glossary: "Node" exists in HW101 with the circuit meaning; this course's entry is
  "Node (machine)", and the chapter says so (guide 10.1 one meaning per word).
- Source ids in F6-37 skip D2 (D1, D3–D6): harmless, ids are labels.

### F6-38 Fused kernels: online softmax and layer norm (written in the first run)
- Reviewed in the second run and kept unchanged. Listings: `softmax.cpp`, `layernorm.cpp`,
  `fused_trace.cpp`, `worked.cpp`, forensic and forensic_flat runs (pass);
  `softmax_fused.cu` (built; untested on hardware); `ptx_softmax`, `resources` (compiled
  only). This chapter carries the course card's forensic lab "NaN after fusion".
- Unverified boxes: lowering of `__expf` to `ex2.approx` and its accuracy; cuBLASLt /
  CUTLASS epilogue examples; untested on hardware.
- Glossary: "Catastrophic cancellation" is already defined in MA202 (consistent wording);
  not duplicated in this course's glossary.json.

### F6-39 A tiled attention forward pass
- Listings run: `attention.cpp` (pass; unfused vs tiled with traffic counters),
  `attn_fwd.cu` (built; untested on hardware), `resources`, `sass_loop` (compiled only),
  `forensic` (pass).
- Worked-example and check-yourself numbers were recomputed with a scratch Python script
  and with extra runs of Listing 1 (pad 31/32), not stored in the lab folder.
- Unverified boxes: (1) the differences between the teaching kernel and published
  FlashAttention/FlashAttention-2 kernels (papers not opened); (2) shared-memory
  broadcast being conflict-free and `MUFU.EX2` on the SFUs (from earlier chapters'
  sources, not SASS documentation); (3) untested on hardware; Listing 2 has no masking.

### F6-40 The same kernels in Triton: a productivity comparison (milestone E11)
- **Triton, PyTorch and a GPU are absent.** `triton_kernels.py` is syntax-checked only
  (`pycheck`, pass) and the import attempt fails as expected (`triton_import`, exit 1,
  marked untested). Every Triton name and every claim about the Triton compiler is in an
  unverified box at the top of the chapter and in Figure 2's caption.
- Run for real: `tile_model.py` (the university's NumPy model of the block-program
  style: sum, softmax, matmul; pass), its forensic mode (pass), `loc.py` (lines of code,
  pass).
- Analogy proposal: **Triton = a recipe card written for a whole table instead of for one
  helper**; the "head of the hall" stands for the compiler (and "Where the analogy
  breaks" says it is compile-time, not a run-time manager).
- Decision for the owner: whether the Lab may require installing Triton/PyTorch in a
  virtual environment (the chapter says so) and which Triton version the dossier will
  record.

## Decisions left to the owner

1. Approve publishing with the unverified boxes listed above (AH-19).
2. Register the two analogy proposals (library station; table-level recipe card) and the
   invented TN-4 node.
3. Source Researcher: open the CUDA Programming Guide sections on cooperative groups,
   peer-to-peer access and IPC, the cuBLAS/CUB/Thrust/Triton documentation and the
   FlashAttention papers, and close gate G1 for these six chapters.
4. Lab Engineer with GPU access: run every `.cu` listing on a real NVIDIA GPU (and
   F6-37 on a multi-GPU node), then replace the "untested on hardware" boxes with real
   records.
