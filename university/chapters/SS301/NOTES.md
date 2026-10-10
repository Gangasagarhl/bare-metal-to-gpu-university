# SS301 — Platform security: chains of trust: author notes

Chapters F11-01 to F11-06, level L3, 3 credits. Prerequisites: OS301, OS305. Maps to Curriculum 2.5, C13 and H4.
The labs are in `university/labs/F11-01` to `university/labs/F11-06`.

Each lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All six were run again, one after another, at the end of this build on 2026-10-10, after the last code change. The table below comes from the `.log` files of that final run.

## Toolchain (as recorded in the logs)

- **Host models:** g++ 13.3.0 with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined`, and OpenSSL 3.0.13 (libcrypto, plus the command-line tool for cross-checks). Python 3.13 hashlib is used for independent recomputation.
- **UEFI programs (F11-02, F11-03):**
  - Built with clang 18.1.3 (`--target=x86_64-unknown-windows`) and lld-link 18.1.3.
  - Run on QEMU 8.2.2 q35 with `smm=on` and secure pflash.
  - Firmware: OVMF from the ovmf package 2024.02-2ubuntu0.10, using `OVMF_CODE_4M.secboot.fd` and a fresh copy of `OVMF_VARS_4M.fd` for every run.
  - Keys, signed variable updates and Authenticode signing use python3-cryptography 50.0.1.
- **TrustZone (F11-05):**
  - Built with clang 18.1.3 and ld.lld for `aarch64-none-elf`, plus llvm-objcopy.
  - Run on QEMU 8.2.2 `qemu-system-aarch64 -machine virt,secure=on -cpu cortex-a57`, with semihosting used only to end each run.
- **Not available in this build:** swtpm and tpm2-tools; MCUboot and imgtool; sbsigntools; TF-A; OP-TEE. The chapters say so wherever it matters. Each one is replaced by:
  - F4-13's `mock_tpm.py`;
  - the university's own image format (`fwimage.h`);
  - F3-16's `sign.py`;
  - a 100-line EL3 monitor.
- **Exit codes:**
  - UEFI runs: 33 means the program ran and wrote 0x10 to `isa-debug-exit`. 124 means the firmware refused the image: `qemu_run.py` stopped QEMU at the firmware's "Press any key to enter the Boot Manager" line.
  - F11-05 runs: semihosting SYS_EXIT carries the code the program asked for.

## Listings run

**Nothing was run on real hardware.** There was no real TPM, no microcontroller, no Arm board, no robot and no drone.

Status values:
- **pass:** the expected result.
- **expected-refusal:** the firmware is supposed to refuse here, and `run.sh` checks that it does.
- **untested on hardware:** applies to every emulator run and every device model.

| Chapter | Run | Exit | Status |
|---|---|---|---|
| F11-01 | chain | 0 | pass (host model) |
| F11-01 | crosscheck | 0 | pass |
| F11-01 | forensic | 0 | pass (host model) |
| F11-02 | keys | 0 | pass |
| F11-02 | sb_setup | 33 | pass; untested on hardware |
| F11-02 | enroll | 33 | pass; untested on hardware |
| F11-02 | sb_unsigned | 124 | expected-refusal; untested on hardware |
| F11-02 | sign_db1 | 0 | pass |
| F11-02 | sb_db1 | 33 | pass; untested on hardware |
| F11-02 | sb_snakeoil | 124 | expected-refusal; untested on hardware |
| F11-02 | forged | 33 (SetVariable → Security Violation) | pass; untested on hardware |
| F11-02 | replay | 33 (SetVariable → Security Violation) | pass; untested on hardware |
| F11-02 | rotate | 33 | pass; untested on hardware |
| F11-02 | forensic_boot | 124 | expected-refusal; untested on hardware |
| F11-02 | forensic_signer | 0 | pass |
| F11-02 | sb_db2 | 33 | pass; untested on hardware |
| F11-03 | measured | 0 | pass (host model); untested on hardware |
| F11-03 | crosscheck | 0 | pass |
| F11-03 | forensic | 0 | pass (host model) |
| F11-03 | fw_empty | 33 | pass; untested on hardware; mock TPM |
| F11-03 | fw_enrolled | 33 | pass; untested on hardware; mock TPM |
| F11-03 | fw_rotated | 33 | pass; untested on hardware; mock TPM |
| F11-03 | pcr_compare | 0 | pass |
| F11-04 | updates | 0 | pass (host model); untested on hardware |
| F11-04 | crosscheck | 0 | pass (OpenSSL CLI: "Verified OK", then "Verification failure") |
| F11-04 | forensic | 0 | pass (host model) |
| F11-05 | tz | 0 | pass; untested on hardware |
| F11-05 | tz_control | 1 | pass (exit 1 = the control read succeeded, as intended); untested on hardware |
| F11-05 | crosscheck | 0 | pass |
| F11-05 | forensic_peek | 0 | pass (the leak is the expected forensic evidence); untested on hardware |
| F11-05 | fixed_peek | 0 | pass; untested on hardware |
| F11-06 | robot | 0 | pass (host model); untested on hardware |
| F11-06 | forensic | 0 | pass (host model) |

## Shared code and dependencies between lab folders

- **Copies with new headers.** Each copied file is marked as a copy in its header:
  - `sha256.h` in F11-01, F11-02, F11-03, F11-04 and F11-05 is copied from F3-41.
  - `efi.hpp`, `console.hpp` and `qemu_run.py` in F11-02 are copied from F3-10. `efi.hpp` was extended with SetVariable, more status codes and the db GUIDs.
  - `sign.py` in F11-02 is copied from F3-16, with "-" for an unencrypted key and `--signer` added.
  - `mock_tpm.py` in F11-03 is copied from F4-13. The code is unchanged; only the docstring was adapted.
  - `fwimage.h` in F11-06 is copied from F11-04. Only the header comment was changed.
- **F11-03 depends on F11-02's folder.** Its `run.sh` uses these files from `../F11-02`:
  - `mkvars.py`, `setvars.cc`, `sign.py`, `qemu_run.py`;
  - `efi.hpp` and `console.hpp`;
  - the persistent `keys/`.

  Whenever F11-02's keys or tools change, re-run F11-02 and then F11-03.
- **Test keys are committed on purpose:**
  - `F11-02/keys/` (PK, KEK, db1, db2: RSA keys and certificates);
  - `F11-04/keys/` and `F11-06/keys/` (P-256 PEM).

  The chapters quote their fingerprints and key hashes, so the keys must stay stable. Each chapter's Safety box says these are public test keys that must never be enrolled or used for a product. Regenerating them changes many numbers in F11-02, F11-03, F11-04 and F11-06.

## Analogy proposals (F11: a castle with gates and guards)

The registered mapping is used as is: chain of trust / secure boot = each gate guard checks the badge of the next person. It breaks where badges are cryptographic signatures and a stolen key breaks everything after it.

The following new mappings are **proposals** for the analogy registry:

- **F11-01**
  - Boot ROM = the stone guard carved into the first gate, who cannot be replaced or bribed.
  - The hall = the running application.
- **F11-02**
  - PK = the castle's owner, who appoints the stewards.
  - KEK = the stewards, who keep the wall lists.
  - db = the guest list of seal-makers.
  - dbx = the banned list, which wins over the guest list.
  - Setup mode = a new castle with no owner yet, where anybody may nail anything to the wall.
- **F11-03**
  - TPM = the castle's notary in a locked tower, who keeps a tally that can only grow.
  - Event log = the open visitors' book.
  - Quote = the notary's sealed parchment, carrying the king's messenger's fresh password (the nonce).
  - Sealing = a box the notary opens only while the tally reads what it read when the box was closed.
- **F11-04**
  - Update = sealed orders delivered by courier.
  - Unprotected metadata = what is scribbled on the envelope.
  - Security counter = a notch on the gatepost that can be deepened but never filled in.
- **F11-05**
  - TrustZone = the inner keep.
  - Secure monitor = the guard behind the hatch of the keep's only door.
  - SMC = a request passed through the hatch.
  - Confused deputy = the keep's servant who reads any shelf whose number is passed in.
- **F11-06**
  - A robot = a castle with several towers, each with its own guard and seal impression.
  - The main computer = the captain who lowers the drawbridge (enables the motors) only when every tower reports a checked guard and the towers' orders belong together.

## Glossary

`glossary.json` has 29 new terms. Every chapter's jargon box links its own terms. Terms already defined by other courses are linked and not redefined:

- **DR302:** TPM, PCR, extend, event log, attestation and quote, root of trust for measurement.
- **OS301:** measured boot, Secure Boot (PK, KEK, db, dbx).
- **OS305:** MCUboot, anti-rollback, test and confirm, digital signature, cryptographic hash, boot loader (microcontroller).
- **DR403:** exception level, EL3 runtime (BL31), SMC, secure and non-secure worlds, TF-A, boot chain (Arm).
- **HW303:** JTAG, debug probe.
- **HW101:** fuse.
- **Other courses:** safe state, ESC, threat model.
- **F3-16:** Authenticode, shim.

Possible overlaps to check:
- "Trusted computing base (TCB)" and "TOCTOU" (F11-01) may also be proposed by SS302 or SS401.
- "Threat model" is linked from F11-06, but it is defined in SS401's glossary, which was being written at the same time.

## Per chapter: unverified claims, boxes and open points

All D-sources are "title only — not opened during this build (dossier gate G1 open)". No document was opened apart from the local curriculum, the guide and the earlier chapters of this university.

### F11-01 The castle gate: chains of trust
- **Unverified box:** how a specific chip's boot ROM finds and checks the first loader (fuses, algorithm, failure behaviour, recovery-mode bypass).
- **Evidence:** the chain model is the university's own host model, cross-checked with an independent SHA-256 run.

### F11-02 Secure Boot keys and databases
- **Unverified boxes (3):**
  - GUIDs, attribute values, EFI_TIME and WIN_CERTIFICATE_UEFI_GUID layout, and what the signature covers. All are from memory of the UEFI specification; the firmware accepted the updates.
  - How vendors and operating systems deliver db and dbx updates; retail PC certificates; shim revocation.
  - Untested on hardware: the SMM-protection explanation is from the curriculum and memory.
- **Observed but not read in the specification:** dbx wins over db.

### F11-03 Measured boot and the TPM
- **Unverified boxes (3):**
  - The PCR assignment, the event type names and numbers, the TCG2 log layout, and the EFI_TCG2_PROTOCOL slot order and GUID.
  - The real TPM2_Quote structure and attestation-key certification. Quotes and sealing exist only in the OpenSSL model.
  - No real TPM; `mock_tpm.py` has no keys, quotes, sealing or authorisation, and its control protocol is from memory.
- **The C13 acceptance idea is shown with the mock TPM, not swtpm.** Replay equals the TPM's PCRs for all eight PCRs in three boots. The TPM's values are read from the mock's own record, not through your own kernel's driver (F4-13 covers the driver).
- **Unidentified events:** four PCR 1 events of type 0x0000000a are printed unnamed.

### F11-04 Signed firmware updates
- **Unverified boxes (3):**
  - Everything about MCUboot itself: format, TLV types, protected and unprotected TLVs, security-counter TLV, swap modes, confirmation, and imgtool.
  - Key-revocation patterns.
  - Untested on hardware; the ECDSA timing range is from general knowledge.
- **The course lab "MCUboot signature rejection test (H4)" was run on the university's model.** The steps for a real board are given as an untested procedure.
- **Owner decision, H4 wording.** H4 says "power loss during an update (simulated) leaves the old image running".
  - The model never leaves the device without a working image: 26 of 26 cut points recovered.
  - But it finishes the interrupted swap and runs the *new* image in trial mode. The old image is kept until the new one confirms and returns if the new one fails.
  - The chapter states this in a Watch out box. Please decide whether H4 means this literally, in which case the model would need to undo the swap after a cut.

### F11-05 TrustZone, TF-A and OP-TEE
- **Unverified boxes (4):**
  - SCR_EL3 and SPSR bit values, vector offsets, ESR decoding, the QEMU virt memory map, and the SMCCC function-ID layout.
  - TF-A's trusted boot.
  - OP-TEE's normal-world side and TA signing.
  - Untested on hardware; the bus-level description and protection controllers are from memory.
- **TF-A and OP-TEE were neither built nor run.** The course goal "TrustZone with TF-A and OP-TEE" is met at the level of mechanism (own monitor) and placement (stage map from the curriculum). Building TF-A and OP-TEE for QEMU is left as an extension, because neither is available offline here.
- **Owner decision:** should a later build with network access add a real TF-A and OP-TEE lab on QEMU?

### F11-06 Secure boot on robots and drones
- **Unverified boxes (2):**
  - Chip-specific secure-boot, debug-lock and revocation features.
  - Untested on hardware; no robot-bus authentication or flight-stack security sources were read.
- **Exam P and the course project** are specified in the Mini-project section, with a rubric.
- **Owner decision:** where should exams Q and F (quiz and forensic) live? The chapters' Check yourself sections and forensic labs can serve as their item pool. No separate exam page was written, because the fragment format has no exam section.
- **Safety:** the Safety boxes follow the guide's rules (propellers off, kill switch, LiPo handling, supervision for L0–L2, aviation authority). No numbers are given from memory.

## Decisions for the owner (summary)

1. **H4's power-cut wording** (F11-04): finish the swap or undo it?
2. **A real TF-A + OP-TEE lab** on QEMU when network access is available (F11-05).
3. **swtpm instead of `mock_tpm.py`** for C13 when available (F11-03), so that quotes and sealing can run on a real TPM implementation.
4. **Committed test keys** in three lab folders. They are kept on purpose; please confirm.
5. **Exams Q and F:** use the chapters' Check yourself and forensic labs as the item pool, or write a separate exam page?
6. **Analogy proposals** above: accept, change or reject.
