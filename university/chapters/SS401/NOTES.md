# SS401 — Threat modelling: author notes

Chapters F11-13 to F11-17. Level L4, 2 credits. Prerequisites SS301 and SS302.
The course card maps to Shostack, "Threat Modeling". It names no curriculum milestone, so
no milestone acceptance tests are quoted. The related curriculum item is runbook 5.4, row
"Trust", which F11-14 quotes.
Labs are in `university/labs/F11-13` to `university/labs/F11-17`.

## Build state

- **Labs.** All five labs pass `university/labs/run_lab.sh university/labs/<ID>` with exit
  code 0. All five were re-run at the end of this build, on 2026-10-10. Every step's
  `.log` records exit code 0.
- **Fragment checks.** Every fragment passes these checks:
  - Python `html.parser` balance;
  - ids prefixed with the chapter id;
  - no URLs and no `<script>`;
  - all 21 sections in order, plus Answers and the forensic key;
  - every `data-src` and `data-run` file exists;
  - every source tag resolves and every source is used;
  - at least two inline SVG figures;
  - no phrases from guide 13.8.
- **build.py.** `python3 university/build/build.py` prints no PROBLEM line that mentions
  SS401 or F11-13 to F11-17, and none for this course's glossary anchors. The remaining
  PROBLEM lines are other courses' broken `#gl-` links.
- **Links to unwritten chapters.** F11-06, F11-12 and some SS302 chapters were not written
  yet. The builder rewrites those links to their catalogue rows (`#cat-…`), and they will
  resolve when the chapters exist.

## Listings run (toolchain from the logs)

g++ 13.3.0 with `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`.
Python 3, GNU bash 5.2.21 and GNU coreutils 9.4 were also used.

| Chapter | Listing / step | Result |
|---|---|---|
| F11-13 | `attack_tree.cpp` + `.in` (attack-tree evaluation); `listeners.cpp` (finds its own listener in `/proc/net/tcp`); `forensic.cpp` (evidence) | pass |
| F11-14 | `tm.h` + `stride.cpp` + `.in` (the threat-modelling engine; plant-watering robot); `forensic.cpp` (two diagrams; one engine run inside it exits 1 on purpose, and the program itself exits 0) | pass |
| F11-15 | `stride` (robot DFD, 90 threats); `selftest.cpp` and `crosscheck.py` (the teaching SHA-256 and HMAC agree byte for byte with Python's hashlib and hmac, checked by `run.sh`); `cmd_auth.cpp` (three policies); `forensic.cpp` (replay after restart) | pass |
| F11-16 | `stride` (drone DFD, 104 threats); `ulink.cpp` (toy signed link); `whosent.cpp` (writes `rx_log.csv` and `gcs_tx.csv`); `run.sh`: `samecrypto` (`cmp` with `../F11-15/hmac.h`) and `seqcheck.py` (answer-key analysis) | pass |
| F11-17 | `stride` (cluster DFD, 95 threats); `reach.cpp` + `.in` (reachability, re-run after each fix); `forensic.cpp` (permission model); `run.sh` → `umask_demo.sh` (real `stat` output under umask 0022 and 0077) | pass |

**Expected-fail listings.** There are none.

**Untested on hardware.** Every robot, drone, radio and cluster step is untested on
hardware. The labs mark these steps "(… untested in this build)". The hardware sections
carry unverified boxes that name what would be needed: the course robot kit, PX4 or
ArduPilot SITL with a ground-station app, a bench drone, and the DS303 virtual cluster.
The build has no robot, microcontroller, radio, SITL, Slurm or BMC.

**Shared files.**
- `tm.h` and `stride.cpp` are byte-identical copies in F11-14, F11-15, F11-16 and F11-17.
  Their md5 sums were checked. If you edit one, copy it to all four.
- `hmac.h` is identical in F11-15 and F11-16. F11-16's `run.sh` fails if the two copies
  differ, so F11-16's lab reads `../F11-15/hmac.h`.

**In-text "predict, then run" answers.** These were checked by real runs in scratch copies:
- F11-13 Q7: the extra MITIGATE line gives 12 points on the buttons branch.
- F11-14 Q7: Schedule store moved to zone `home`.
- F11-15 Q7: `kTagBytes = 2` leaves the verdicts unchanged.
- F11-16 Q7: edited frame with a fresh timestamp gives "bad tag".
- F11-17 Q7: `allow login bmc2` gives bmc2 at 2 hops in all three searches.

## Unverified boxes (AH-18)

- **F11-13**
  - The wording of Shostack's four questions, the four kinds of decision (mitigate,
    eliminate, transfer, accept) and where they appear in the book.
  - The layout of `/proc/net/tcp` beyond the fields used. The state code 0A is supported
    by `netinet/tcp.h` from libc6-dev 2.39, which was opened in this build, and by the run.
- **F11-14**
  - The STRIDE-per-element chart: external entity S, R; process all six; store T, I, D,
    plus R for logs; flow T, I, D. Also the STRIDE-per-interaction variant. Both come
    from memory. They are encoded in `tm.h` `lettersFor` (lines 173–182) and in Figure 2.
    Correct all three together.
- **F11-15**
  - ROS 2 security: DDS Security and SROS 2 names, steps and defaults.
  - Untested on hardware: the cost of HMAC on the kit's microcontroller, the serial frame
    format, and the e-stop wiring.
  - A warning box says that the constant-time comparison is not guaranteed by the language.
- **F11-16**
  - MAVLink details: the sequence, system and component ids; that MAVLink 1 cannot be
    signed; MAVLink 2 signing (link id, 48-bit timestamp, 48-bit SHA-256-based signature,
    and whether that is HMAC); the per-stream timestamp state; whether signing is on by
    default; PX4 and ArduPilot handling of unsigned frames.
  - Untested on hardware: radios, RC binding, and the signing cost on a flight controller.
- **F11-17**
  - Slurm and MUNGE authentication (the plugins and the credential contents).
  - Whether GPU memory is cleared between jobs, and how GPU sharing isolates jobs.
  - Untested on a real cluster: only the umask demonstration ran on real Linux.

## Sources used (all title only, dossier gate G1 open, except the runs and L1)

- D1: Shostack, "Threat Modeling: Designing for Security" (registry 4.9). Used in all chapters.
- D2: the MAVLink Developer Guide (F11-16).
- D3: NIST FIPS 180-4, IETF RFC 2104 and NIST FIPS 198-1 (F11-15). Correctness of the
  implementation rests on the cross-check run (R2), not on these sources.
- D4: POSIX, IEEE Std 1003.1 (F11-17), for umask.
- L1 (F11-13): `/usr/include/netinet/tcp.h`, opened in this build.
- C-sources (maps, AH-4):
  - the guide's SS401 card and safety statement;
  - curriculum 5.4 "Trust";
  - university chapters F10-12, DS303 (F5-22 to F5-24), and the OS305, DS304 and DS401
    glossary entries.

## Decisions for the owner

1. **Course-card forensic: "a MAVLink log".**
   - **Problem:** real MAVLink logs cannot be produced in this build (no SITL, no pymavlink).
   - **What I did:** the forensic lab uses logs from the course's own simulated link,
     "ULink". They have MAVLink-like fields (system id, component id, 8-bit sequence) but
     are explicitly not MAVLink.
   - **For you:** approve this, or schedule a SITL-generated MAVLink log (SITL plus a
     second sender) when SITL is available. F11-16 lab step 5 describes it.
2. **Teaching crypto.**
   - **Problem:** the labs needed a MAC, and HMAC had to be written for them.
   - **What I did:** SHA-256 and HMAC are implemented in `hmac.h` for teaching. They are
     cross-checked against Python. The chapter warns against product use.
   - **For you:** approve, or require the labs to call a library such as OpenSSL. That
     would add a build dependency that run_lab.sh does not have.
3. **Course vocabulary for claimed controls.**
   - **What I did:** `tm.h` uses `auth`, `mac`, `audit`, `enc`, `ratelimit` and
     `leastpriv` to mark controls that a diagram claims. This is the course's own
     vocabulary, not a standard.
   - **For you:** keep it, or replace it with a vocabulary from the book once the book is
     checked.
4. **No numeric risk scoring.**
   - **What I did:** the course deliberately avoids numeric risk formulas such as
     probability times impact. It ranks threats by boundary first, then by assets and
     attack trees, and gives the reason in F11-13 Layer 3.
   - **For you:** confirm this is wanted.
5. **Practical exam (P).**
   - **What I did:** F11-17 describes the format of the timed threat-model exam. The time
     limit is left to the exam paper, and no paper or key was written.
   - **For you:** the Exam Writer still has to produce the paper and its key in `_keys/`.
6. **Chapter length.** The chapters are about 5,800–6,500 words of page text each,
   including tables. Prose alone is near the L4 target of about 5,000.

## Analogy mappings used beyond the registry (proposals for the Dean, guide 8.2)

The registry row for F11 is "Threat model: thinking like a burglar about your own house;
a castle with gates and guards". Within that world the chapters use these mappings, and
each needs registering:

| Term | Mapping |
|---|---|
| Trust boundary | the castle wall and its gate; the front door of a flat |
| STRIDE | six questions asked at every gate |
| MAC | a wax seal only the castle's ring can make |
| Replay | yesterday's sealed order handed in again |
| Freshness | the guard's note of the last order number |
| Radio link | a gate in the open air; shouting across a field |
| Jamming | shouting so loudly no messenger is heard |
| Multi-tenant cluster | a block of flats with a shared laundry room |
| Shared storage | the laundry room |
| Management network / BMC | the caretaker's office and master key |
| umask | bags left closed unless opened on purpose |
| Insider | a neighbour in the laundry room |

**Glossary qualifiers (one meaning per word).**
- "Entry point (attack surface)" is separate from SP301's "Entry point" (the ELF entry
  address).
- "STRIDE (threat categories)" is separate from HW203's "Stride".
- "Message authentication code (MAC)" names "MAC (media access controller)" as the other
  meaning.
