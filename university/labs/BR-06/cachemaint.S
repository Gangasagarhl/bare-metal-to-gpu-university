/* cachemaint.S - BR-06: the instructions a driver uses around DMA on a non-coherent device,
 * and the barriers that order device accesses, assembled for three CPUs (run.sh shows the
 * encodings). Meanings from the architecture manuals (title only, pending verification);
 * assembling proves only that the instruction exists, not what it does. */
#if defined(__aarch64__)
        dc      cvac, x0        /* clean data cache line by address to the point of coherency */
        dc      ivac, x0        /* invalidate data cache line by address */
        dc      civac, x0       /* clean and invalidate */
        dsb     sy              /* wait until those operations have completed */
        dmb     oshst           /* order earlier stores before later stores (outer shareable) */
#elif defined(__riscv)
        cbo.clean  (a0)         /* Zicbom: clean the cache block that holds address a0 */
        cbo.inval  (a0)         /* Zicbom: invalidate it */
        cbo.flush  (a0)         /* Zicbom: clean and invalidate */
        fence   rw, rw          /* order memory accesses */
        fence   w, o            /* order memory writes before later device (output) writes */
#elif defined(__x86_64__)
        clflush (%rdi)          /* write back and invalidate the line (rarely needed for DMA on PCs) */
        mfence                  /* full memory barrier */
        sfence                  /* store barrier */
#endif
        .section .note.GNU-stack, "", %progbits
