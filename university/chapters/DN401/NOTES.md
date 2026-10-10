# DN401 — build notes (Author and Lab Engineer)

Course: DN401 Simulation, failsafes, regulations and field testing (L4–L5).
Chapters: F10-33, F10-34, F10-35 (L4), F10-36, F10-37 (L5), F10-38 (L4).
Toolchain for every run: g++ 13.3.0, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g
-fsanitize=address,undefined`, through `university/labs/run_lab.sh university/labs/<ID>`.

## Listings run

All six lab folders pass `run_lab.sh` (exit 0). 17 programs; 15 exit 0, and 2 forensic
programs exit 1 on purpose (their verdict is a finding). Outputs were checked twice, and both
runs gave the same bytes. Every program uses the shared course simulator
`university/labs/F10-33/dronesim.hpp`, which is deterministic.

| Chapter | Program | Exit | Role |
|---|---|---|---|
| F10-33 | sitl_mission | 0 | SITL square mission + acceptance tests A1–A6 (all PASS) |
| F10-33 | lockstep | 0 | lockstep (identical) vs free-running (varying) runs |
| F10-33 | ci_flaky | 0 | forensic evidence: flaky CI test |
| F10-34 | hitl_link | 0 | HITL split with two threads and a socketpair, own framing + CRC-16 |
| F10-34 | hitl_delay | 0 | step response vs loop delay 0–600 ms |
| F10-34 | hitl_forensic | 0 | forensic evidence: saturated HITL link |
| F10-35 | failsafe_matrix | 0 | failsafe injection matrix F1–F7 (all PASS) |
| F10-35 | rc_forensic | 0 | forensic evidence: receiver holds output on loss |
| F10-35 | reserve | 0 | return-charge arithmetic |
| F10-36 | safety_case | 0 | safety-case checker on the course case (0 findings) |
| F10-36 | forensic_case | 1 (by design) | same checker on Team B's case (7 findings) |
| F10-37 | gonogo | 0 | readiness review on the course record (GO) |
| F10-37 | forensic_gonogo | 1 (by design) | same review on Team B's record (NO-GO) |
| F10-38 | gen_logs | 0 | writes the two forensic CSV logs |
| F10-38 | heading_margin | 0 | heading-error stability boundary (59.2°) |
| F10-38 | logscan | 0 | log analyser on both logs |
| F10-38 | show_rows | 0 | evidence excerpt |

One extra run did not go into a lab log: F10-38's answer to Check-yourself question 5 quotes
54.4° for `kv = 3.6`. That number comes from a modified copy of heading_margin.cpp compiled in
the scratch folder.

## Untested on hardware (marked in the chapters)

- PX4 and ArduPilot SITL and HITL (F10-33, F10-34): no flight stack was built in this build.
- HITL with the bench flight controller (F10-34 lab part B).
- Bench failsafe tests with props removed (F10-35 lab part B).
- Supervised outdoor hover (F10-37 lab part B). It is allowed only after the R3 sign-off,
  under the authority's current rules, with permission and supervision.
- Analysing a real flight-stack log (F10-38 lab part B).

## Unverified boxes

- F10-33: PX4/ArduPilot SITL commands, simulators, lockstep options and parameters.
- F10-34: HITL parameters and airframes; the MAVLink messages used for HITL; ArduPilot's
  equivalent. Also the link rate, USB behaviour and buffer sizes: 115,200 bit/s is only an
  example setting.
- F10-35: failsafe parameter names, defaults and priority rules in PX4/ArduPilot. Also the
  receiver behaviours on signal loss, and current-sensor and voltage-divider details.
- F10-36: the topics that rules often cover. No rule content is given anywhere. Also how flight
  stacks expose a firmware string and a configuration id.
- F10-37: PX4/ArduPilot pre-arm and arming checks.
- F10-38: log message and field names, rates and tools. Also logging back-ends and the
  recommended compass placement. The "compass-motor compensation" procedure is mentioned as
  "check yours" in the key.

## Decisions and conventions

- Shared simulator: `labs/F10-33/dronesim.hpp`, namespace `dn`.
  - It has an invented battery (OCV curve, R_int 0.025 Ω), a power model of 265·T^1.5 + 8 W,
    drag 0.35 1/s and tau 0.15 s.
  - The compass interference grows with current and is fixed in the body frame.
  - The vehicle has no yaw dynamics.
  - Parameter names (`rc_timeout`, `batt_low_pct`, ...) belong to the course only. The
    chapters say so.
- Failsafes only escalate (Mission < Hold < RTL < Land). Side effect: a breach of a
  lower-rank failsafe is not logged. F10-38 Log A uses this on purpose (fence breach at
  177 m, no event).
- Course text formats were invented for teaching: safety-case and readiness-record `.in` files,
  and the CSV log with `#` header lines.
- The freshness rule (doc 1 year, day 12 h, field 60 min) is the course's teaching rule. It is
  not a recommendation from any document.
- Story continuity across chapters: Team B's v1.5/91be build with new power wiring appears in:
  - the F10-36 forensic case (hazard H5, unargued);
  - the F10-37 forensic record (2026-09-27, NO-GO);
  - F10-38 Log A (flyaway, 177 m).

  Team C's 2,000 mAh capacity setting for a 1,500 mAh pack appears in F10-38 Log B.
- The F10-34 HITL rehearsal uses the course's own framing (`0xA5 | type | seq | len |
  float32 | CRC-16/CCITT-FALSE`), not MAVLink. This is stated in the chapter.

## Analogy proposals (F10 registered: "carrying a tray of drinks while walking")

- Characters used in DN401: Leila (the waiter), Tomás (the friend who narrates the rehearsal)
  and the head waiter. DN301 uses another name (Amara) for the same F10 setting. **Owner
  decision:** unify the F10 character names, or accept a different cast per course.
- Proposed mappings:
  - SITL = Tomás narrates the rehearsal; lockstep = he waits for her reaction; free-running =
    he talks on regardless.
  - HITL = the real tray and shoes, narration over a phone line; link delay = the delay on
    the line.
  - Failsafes = the head waiter's three rules. This includes the registered "if you get
    lost, go back to the meeting point".
  - Safety case = Leila's sheet for the head waiter.
  - Go/no-go = the three-second look at the kitchen door.
  - Log forensics = reconstructing the spill from the stains.

## Glossary

`glossary.json` has 33 entries, generated from the chapters' jargon boxes, with the four
parts, sources, related terms and chapters.

- "Software in the loop (SITL)" is also defined by DN301 (F10-17). build.py merges duplicates
  and the first course's text wins. DN401's entry only adds F10-33 to the chapter list.
- Terms that other courses own are linked, not redefined:
  - Frame (serial link), Checksum (frame check), Delay margin, Phase margin, Bench test;
  - Failsafe (RC loss), RC transmitter and receiver, Telemetry radio, LiPo battery;
  - Internal resistance and voltage sag, Magnetometer, Fault and failure, Checklist;
  - Deterministic simulation, Sim-to-real gap.
- SS402 (written in parallel) defines Hazard, Risk, Harm and Hazard log. F10-36 links to the
  chapters F11-18 and F11-19 instead of to glossary anchors, because SS402's glossary.json did
  not exist when this was written. To avoid a clash with a generic "Safety case" entry that
  SS402 is likely to add, DN401 names its entries "Safety case (flight operation)" and "Risk
  matrix (severity × likelihood)". **Owner decision:** merge these into SS402's entries later
  if wanted.

## Sources outside the guide's registry (proposed for addition)

- GSN Community Standard (F10-36 D2): cited by title only.
- Atul Gawande, "The Checklist Manifesto" (F10-37 D2): cited by title only.
- Åström and Murray, "Feedback Systems" (F10-34 D3, F10-38 D4): cited by title only. Check
  whether it is in the F10/MA registry.

None of the documents were opened during this build (dossier gate G1 open). No URL appears in
any chapter.

## Decisions for the owner

1. Approve the course text formats (safety case, readiness record, CSV log) and the freshness
   rule, or replace them.
2. Unify the F10 character names across DN301 and DN401.
3. Add the three sources above to the registry, or replace them.
4. Decide whether "Safety case (flight operation)" and "Risk matrix (severity × likelihood)"
   stay separate from SS402's terms.
5. The PX4/ArduPilot HITL, failsafe and log details stay in unverified boxes until someone
   opens the documentation for the release that MP7 targets.
