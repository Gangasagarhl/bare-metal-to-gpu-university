# BR-05 From bare metal to an RTOS to Linux — author's notes

Bridge chapter, L3, placed inside OS305 between F3-38 and F3-39; referenced by RB304 and DN302.
Fragment: `university/chapters/BR-05/BR-05.html`; lab: `university/labs/BR-05/`
(run `university/labs/run_lab.sh university/labs/BR-05` from the repository root; exit 0).

## Listings run (all in this build)

| Step | Listing(s) | Result |
|---|---|---|
| core_test | loop_core.h, core_test.cpp (g++ 13.3.0, ASan+UBSan) | pass; checksum 0x941c0934 |
| bm_superloop | bm_superloop.cc on QEMU mps2-an385 -icount | pass; untested on hardware |
| bm_irq | bm_irq.cc on QEMU | pass; untested on hardware |
| rtos, repeat | urtos.cc + rtos_app.cc on QEMU, run twice | pass, byte-identical; untested on hardware |
| forensic | rtos_app.cc -DBR05_LONG_CRITICAL=1 | pass (lateness is the studied result); untested on hardware |
| trap_isr_block | trap_isr_block.cc | expected hang: exit 124 (10 s limit); untested on hardware |
| trap_isr_fixed | trap_isr_block.cc -DBR05_FIX=1 | pass; untested on hardware |
| trap_heap | trap_heap.cc | expected link failure (ld.lld: undefined symbol operator new[]) |
| smoke | linux_loop.cc, linux_inversion.cc with sanitizers | pass |
| linux_other_idle/_load, linux_fifo_load, linux_alloc | linux_loop.cc -O2 | pass; real measurements on the shared VM, change run to run |
| inversion_none / _inherit | linux_inversion.cc | pass (about 116 ms vs 15 ms wait) |
| kernel | uname, /proc/config.gz | pass; shows no PREEMPT_RT |
| footprint, compare | llvm-size/size/ldd; compare.py | pass; all eight checksums equal |

`startup.cc`, `board.h`, `os305.ld`, `urtos.h`, `urtos.cc` are copied unchanged from `labs/F3-40`
(one first comment line added). If F3-40's kernel changes, re-copy and re-run.

## Unverified boxes (2) and claims not verified

1. Layer 3 box: FreeRTOS "FromISR" variants, heap schemes and static allocation; Zephyr zero-timeout
   rule in ISRs, memory slabs, MPU user mode; PREEMPT_RT threaded handlers and sleeping locks with
   priority inheritance; Armv7-M MPU optional with few regions; MCL_FUTURE populating mappings.
2. Lab box: Zephyr/FreeRTOS port, PREEMPT_RT runs and real-board timing untested in this build,
   with what would be needed.
- All D-sources are "title only" (no internet): Armv7-M ARM, Yiu, White, Zephyr/FreeRTOS docs,
  kernel "Real-Time Preemption", man-pages/POSIX, Buttazzo and Liu, QEMU guide, OSTEP. Gate G1 open.
- "POSIX names PTHREAD_PRIO_NONE as the default protocol" is from memory (tagged D6); the program
  sets the protocol explicitly in both runs, so the run does not prove the default.
- The statement that the kernel's preemption build default is "none" is read from the kernel's own
  config (CONFIG_PREEMPT_NONE=y with CONFIG_PREEMPT_DYNAMIC=y); the boot-time mode was not queried.

## Measurements (AH-23)

- Microcontroller numbers are QEMU instruction-counted virtual time (repeatable, not a real chip).
- Linux numbers come from the build container (x86_64 VM, 4 CPUs, shared with other agents' work
  during this build). They varied strongly between runs (e.g. SCHED_FIFO worst case from about
  0.45 ms to 2.8 ms across development runs). The prose quotes the recorded run and states the shape;
  if the lab is re-run, re-check the numbers quoted in Layer 2, Layer 3 (traps 3 and 4) and the
  Worked example step 5 against `compare.out`, `linux_*.out`.

## Decisions for the owner

1. **uRTOS stands in for Zephyr/FreeRTOS** in the card's lab (neither is installed; no network).
   The bridge previews uRTOS before F3-39 builds it. Approve, or provide an RTOS SDK in the container.
2. **PREEMPT_RT half of the lab is untested**: the container's kernel cannot be changed. A machine
   with a PREEMPT_RT kernel (ideally the same hardware with and without) is needed to close it.
3. Chapter length is above the L3 guideline (about 4,000 words of prose): the card asks for each
   trap in depth with a real run, plus three worlds; trimming candidates are the code tables of
   Listings 3, 4 and 12.
4. New analogy uses within the registered F3 world (no new mapping registered): "three schools"
   (one-room school = bare metal, school with a hall rule = RTOS, campus with principal = Linux),
   doorbell = timer interrupt, "locking the hall door from the inside" = interrupt lock. Please
   confirm or register.
5. Forward links to RB304 (F9-45, F9-47, F9-48, F9-49, F9-51, F9-66) and DN302 (F10-20) for the
   Linux real-time depth, instead of repeating their labs here.

## Glossary

New terms in `glossary.json` (checked absent from all other glossaries): Superloop, Actuation
lateness, Interrupt context, Interrupt lock (interrupts-disabled section), Memory protection unit
(MPU), Memory footprint. Existing terms are linked (HW204, OS305, OS201, HW203, SP201, RB304, DN302,
DR402).
