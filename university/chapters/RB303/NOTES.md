# RB303 — ROS 2 architecture and DDS: author notes

Chapters F9-37 to F9-44, level L3, 4 credits. Analogy world F9 (riding a bicycle; a body). Maps to the ROS 2 documentation, OMG DDS 1.4 and OMG DDSI-RTPS 2.5. **The course card gives no curriculum milestone id** (see owner decisions).
Labs are in `university/labs/F9-37` to `university/labs/F9-44`.

Every lab folder passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All eight were run again at the end of this build (2026-10-10), after the last change. The table below comes from the `.log` files of that final run.

## Approach

**ROS 2, any DDS implementation and Gazebo are not installed in the build container, and there is no internet.** So every concept is taught with the university's own small, deterministic C++ models, and the chapters show their real output:

- F9-37: graph and name resolution.
- F9-38: an rclcpp-shaped mini library.
- F9-39: a discrete-event executor and callback groups, plus a std::jthread demo under ThreadSanitizer.
- F9-40: RTPS discovery, reliability and HEARTBEAT/ACKNACK/GAP, plus CDR byte layout.
- F9-41: the QoS RxO matrix, a late joiner and deadline.
- F9-42: parameters and the lifecycle.
- F9-43: bag, replay, plot and trace.
- F9-44: a 2D differential-drive simulator with ray-cast ranges, RTF and two clocks.

Real-API files were written from memory and are shown inside "Not verified" boxes. Each was compile- or syntax-attempted, and the failure is recorded honestly:

- F9-38 `talker_ros2.cxx` with its CMakeLists.txt and package.xml.
- F9-42 `robot.launch.py` and `params.yaml`.

## Toolchain

- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0. The flags are `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. F9-39 also builds with `-fsanitize=thread`.
- Python 3 from the container, used only for the F9-42 syntax check. The version is in `launch_syntax.log`.

## Listings run

| Chapter | Run (`.log`) | Exit / result | Status |
|---|---|---|---|
| F9-37 | graph | 0 | pass |
| F9-37 | nsbug | 0 | pass (forensic evidence) |
| F9-38 | talker_listener | 0 | pass |
| F9-38 | lost_sub | 0 | pass (forensic evidence) |
| F9-38 | talker_ros2 | 1 | expected failure: `rclcpp/rclcpp.hpp` not found; **untested on hardware/ROS 2** |
| F9-39 | executor_sim | 0 | pass |
| F9-39 | starve | 0 | pass (forensic evidence) |
| F9-39 | group_threads | 0 | pass |
| F9-39 | group_threads_tsan | 0 | pass (no race in the shipped version; the lab's race step was checked during authoring) |
| F9-40 | rtps_model | 0 | pass |
| F9-40 | domain | 0 | pass (forensic evidence) |
| F9-40 | cdr_bytes | 0 | pass |
| F9-41 | qos_lab | 0 | pass (course lab: the QoS mismatch experiment) |
| F9-41 | hears_nothing | 0 | pass (course forensic: "Subscriber hears nothing") |
| F9-42 | bringup | 0 | pass |
| F9-42 | tape_test | 0 | pass (forensic evidence) |
| F9-42 | launch_syntax | 0 | Python syntax OK; `import launch` fails (expected); **untested as a launch file** |
| F9-43 | bag_tool | 0 | pass |
| F9-43 | half_distance | 0 | pass (forensic evidence) |
| F9-44 | diffdrive_sim | 0 | pass (course lab: the teleoperated simulated robot with sensor topics) |
| F9-44 | busy_pc | 0 | pass (forensic evidence) |
| F9-44 | busy_pc_simclock | 0 | pass |

Every "Predict, then run" answer and every lab "expected observation" was checked by running the model on a scratch copy, and the answer says so. Where a first draft was wrong, the answer was corrected to the run's output; F9-39 and F9-43 each had corrections.

## Unverified boxes (19 in all)

Each chapter has a Layer 3 box listing the real API, command and parameter names as remembered. Each also has a Part B lab outline for real ROS 2 or Gazebo, marked untested.

- F9-37 (3 boxes):
  - `ros2` CLI names;
  - intra-process and shared-memory transport;
  - the Part B outline.
- F9-38 (2): the rclcpp API, Listing 3 with CMake and package.xml, colcon, and the Part B outline.
- F9-39 (2):
  - executor classes and callback-group API;
  - the events executor and the Part B outline.
- F9-40 (3):
  - RMW implementations and the default vendor;
  - discovery and port numbers (domain ID to UDP ports);
  - the Part B outline.
- F9-41 (2):
  - QoS policy names, the predefined profiles, `ros2 topic info --verbose` output and incompatible-QoS events;
  - the Part B outline.
- F9-42 (3):
  - the parameters, launch and lifecycle API;
  - Listings 2 and 3 (`robot.launch.py`, `params.yaml`);
  - the Part B outline.
- F9-43 (2): `ros2 bag`, rosbag2 storage formats, `use_sim_time`, rqt, PlotJuggler, RViz2, ros2_tracing and LTTng, plus the Part B outline.
- F9-44 (2): Gazebo naming and versions, ROS 2–Gazebo pairings, `gz sim`, SDF elements, plugin names, `ros_gz_bridge` syntax, plus the Part B outline.

All sources are cited title only. Each carries "Title only — not opened during this build … (dossier gate G1 open)". The Source Researcher must confirm the ROS 2 documentation pages, OMG DDS 1.4, DDSI-RTPS 2.5, DDS-XTypes (for CDR), the ros2_tracing paper (Bédard, Lütkebohle, Dagenais, RA-L 2022) and Koenig and Howard (IROS 2004) for Gazebo.

## Analogy mappings proposed (F9 world; for the analogy registry)

The registered mappings are used as given: "node / topic = team members / notice boards" and "DDS = the postal service behind the notice boards". The proposed new ones are below. Story hooks use a cycling club, with a different member's name in each chapter.

| Chapter | Concept | Proposed mapping |
|---|---|---|
| F9-37 | service | asking at the club desk |
| F9-37 | action | a long errand with progress reports and the option to cancel |
| F9-38 | subscription object | a membership card: drop it and the board stops calling you |
| F9-39 | executor | the helpers at the desk |
| F9-39 | mutually exclusive callback group | jobs that share one toolbox |
| F9-39 | reentrant callback group | jobs that each bring their own tools |
| F9-40 | DDS domain | a town with its own post office |
| F9-40 | RTPS sequence number | numbered letters |
| F9-40 | HEARTBEAT / ACKNACK | "I have sent 1–8" / "I am missing 3" |
| F9-41 | QoS request versus offer | registered post versus ordinary post |
| F9-42 | launch file | the ride leader's checklist for the start |
| F9-42 | parameter | the setting on a member's card (saddle height, tyre pressure) |
| F9-42 | lifecycle | bike checked → at the start line → riding → packed away |
| F9-43 | bag / replay | photocopies of every notice with its pin time / pinning them again, possibly faster |
| F9-43 | tracing | a stopwatch log of each helper's jobs |
| F9-44 | simulator / RTF / sim clock | turbo trainer with a screen / the video playing slower / the clock in the video |

## Course card coverage

- **Labs:**
  - "teleoperated simulated robot with sensor topics" is the F9-44 lab (Listing 1 plus `diffdrive_sim.in`, with a Gazebo Part B).
  - "QoS mismatch experiment" is the F9-41 lab.
- **Forensic:** "Subscriber hears nothing" is in F9-41. The verdict-free `endpoints` command plays the role of `ros2 topic info --verbose`: on /map, a TRANSIENT_LOCAL request meets a VOLATILE offer, with distractor depth differences on /scan. Every other chapter also has its own forensic lab.
- **Exam P** ("build a node graph from a specification") is practised in F9-37's lab and mini-project and in the course project.
- **The course project** (simulated drivers, odometry, EKF node, teleop) is specified with a rubric in F9-44's mini-project section. F9-42 contributes the parameter and lifecycle requirements, and F9-43 the bag and replay regression tests.

## Glossary

`glossary.json` holds 57 terms, each with simple, analogy and precise text, a source, related terms and chapters. The text matches each chapter's jargon box exactly. Every `gl-` link in the eight chapters resolves, either here or in another course's glossary: RB101 Simulator, Odometry, Lidar, Differential drive, Emergency stop; SP-level Callback, Lambda expression, Lifetime, CMake, Thread pool, Mutex, Data race, ThreadSanitizer, UDP, Marshalling, Best-effort delivery, Retransmission, Percentile, Tail latency. No anchor clashes with another course's glossary.

## Decisions for the owner

1. **ROS 2 distribution.** The chapters are written to be distribution-neutral, and every name is in unverified boxes. Pick one LTS distribution (for example Jazzy) for the labs' Part B and the course project. The Source Researcher can then check the boxes against that distribution's documentation.
2. **Gazebo release.** The Part B lab and the project need a Gazebo release paired with the chosen distribution. Gazebo Classic should not be used. Confirm whether the faculty prefers Gazebo or allows another simulator (Webots, Isaac Sim) for students without GPUs.
3. **DDS vendor.** F9-40 and F9-41 do not name a default RMW or DDS vendor as fact. Decide whether the course standardises on one (Fast DDS or Cyclone DDS), because QoS defaults and discovery behaviour differ.
4. **No curriculum milestone id.** The course card lists none, so the meta `maps` lines say "no curriculum milestone id". Assign one if the track structure expects it.
5. **Word counts.** Prose ranges from about 4,500 to 5,700 words per chapter, against the L3 target of about 4,000. The extra comes from the unverified boxes and the Part B outlines. Trim if the target is strict.
6. **F9-39 ThreadSanitizer.** The shipped `group_threads.cpp` is race-free, so the TSan run exits 0. The lab asks students to remove the lock and see the race. Decide whether a deliberately racy evidence file should also ship.
