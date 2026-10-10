# RB401 — Mapping, SLAM and planning: author notes

Chapters F9-52 to F9-58, level L4, 4 credits. Prerequisites: RB302, RB303, RB301.
Labs are in `university/labs/F9-52` to `university/labs/F9-58`.

All seven lab folders pass `university/labs/run_lab.sh university/labs/<ID>` with status 0; they were re-run in a final sweep on 2026-10-10 after the last change to any listing. The table below comes from the `.log` files of that sweep. Every fragment passes the html.parser balance check, has all ids prefixed with the chapter id, contains no URL and no `<script>`, and has the 20 h2 sections in template order plus the jargon box and the Transition box (21 template sections plus Answers). `python3 university/build/build.py` reports no PROBLEM line for F9-52 to F9-58 or RB401. The remaining PROBLEM lines in that run belong to other courses.

## Toolchain (as recorded in the logs)

- g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`, on a Linux x86_64 cloud build container.
- No ROS 2, Nav2, MoveIt 2, Gazebo or other simulator is installed in the build container. Every robot, sensor, world and framework in this course is **the university's own C++ code**:
  - a house floor plan and LiDAR simulator (`labs/F9-52/house.hpp`);
  - a scan matcher and pose graph (F9-53);
  - a grid planner (F9-54) and RRT/RRT* (F9-55);
  - a mini navigation stack (F9-56);
  - a planar 3-joint arm pipeline (F9-57);
  - a behaviour-tree engine (F9-58).
- These stand-ins are built to show the parts that Nav2 and MoveIt 2 have. Every Nav2 or MoveIt 2 name, API or parameter in the text is in a "Not verified" box.

## Listings run

| Chapter | Run (`.log`) | Exit | Status |
|---|---|---|---|
| F9-52 | logodds | 0 | pass |
| F9-52 | map_house | 0 | pass (course lab "map a simulated house") |
| F9-52 | phantom | 0 | pass (forensic evidence) |
| F9-52 | phantom_fix | 0 | pass |
| F9-53 | bag_fix | 0 | pass (course forensic fix) |
| F9-53 | bag_lost | 0 | pass (course forensic "The robot that is lost", evidence) |
| F9-53 | pose_graph | 0 | pass |
| F9-53 | slam_lite | 0 | pass |
| F9-54 | astar | 0 | pass (course lab "navigate to goals", planning part) |
| F9-54 | astar_small | 0 | pass (worked example) |
| F9-54 | heuristic_bug | 0 | pass (forensic evidence) |
| F9-55 | cspace | 0 | pass |
| F9-55 | rrt | 0 | pass (has `rrt.timeout` = 60 s because RRT* under the sanitizers is slow) |
| F9-55 | thin_wall | 0 | pass (forensic evidence) |
| F9-56 | nav | 0 | pass (course lab "navigate to goals") |
| F9-56 | nav_bad | 3 | expected failure: two goals aborted (forensic evidence) |
| F9-57 | ik_check | 0 | pass (worked example) |
| F9-57 | no_attach | 3 | expected failure: world check finds a collision (forensic evidence) |
| F9-57 | pick_place | 0 | pass (course lab "pick and place in simulation") |
| F9-58 | bt_bug | 0 | pass: the faulty tree reports SUCCESS, which is the point of the forensic lab |
| F9-58 | fetch_bt | 0 | pass |
| F9-58 | fetch_fsm | 0 | pass |

- **Untested on hardware: all of them.** Nothing ran on a real robot, LiDAR, arm or gripper. No number about real hardware appears in any chapter; every chapter has an "Untested on hardware" box.
- Optional real-tool lab steps (ROS 2, a SLAM package, Nav2, MoveIt 2) are marked "untested in this build". No command or package name is given for them.
- No `.expect-fail` listings.
- The programs are deterministic (fixed seeds), so outputs are identical between sweeps.

## Course card coverage

- **Labs.**
  - "Map a simulated house": F9-52 (`map_house`), extended by F9-53 (`slam_lite`, `pose_graph`).
  - "Navigate to goals": F9-56 (`nav`), with its planning parts in F9-54 and F9-55.
  - "Pick and place in simulation": F9-57 (`pick_place`).
- **Course forensic "The robot that is lost".** F9-53's forensic lab. The bag of `bag_lost` has LiDAR stamps offset from odometry. Straight-only mapping is clean, but turning smears the map (21 far cells). `bag_fix` finds the offset by cross-correlating turn rates (+0.15 s) and the rebuilt map has 0 far cells.
- **Chapter forensics.**
  - F9-52: phantom obstacles from "no return" readings.
  - F9-54: heuristic in millimetres.
  - F9-55: coarse edge check.
  - F9-56: footprint too small.
  - F9-57: object not attached.
  - F9-58: fire-and-forget action.
- **Exam P "configure navigation for a new map".** Rehearsed as the F9-56 mini-project, which asks for a new floor plan, a justified configuration table, goals, log analysis and a safety checklist.
- **Project "Fetch-an-object task in simulation (part of MP6)".** The F9-58 mini-project. It quotes the MP6 milestone verbatim from the guide: "Simulated robot: drivers (simulated), EKF localisation, navigation, one manipulation or delivery task."
- **Curriculum.** SYSTEMS_CURRICULUM has no milestone for robotics mapping, planning or behaviour. The `maps:` lines say so, and they map instead to the course card, the MP6 milestone and the named books and documentation.

## Unverified claims and boxes, per chapter

Every book and paper is cited "Title only — not opened during this build … (dossier gate G1 open)". The Source Researcher must confirm the sections. Tier 2: Nav2 docs, MoveIt 2 docs, BehaviorTree.CPP docs, ROS 2 docs. Tier 3: Probabilistic Robotics, Modern Robotics, Colledanchise & Ögren. Tier 5: papers.

- **F9-52** (3 boxes):
  - ROS 2 occupancy-grid message and map-file format (integer values, unknown value, resolution, origin, image plus description file).
  - Untested on hardware: real LiDAR specifications and the no-return encoding.
  - Lab step 6 (ROS 2, Gazebo, mapping package) untested.
- **F9-53** (4 boxes):
  - ROS 2 header stamps, past-time transform lookup, and message-filter/approximate-time behaviour.
  - Untested on hardware: LiDAR stamping convention and latency.
  - The map → odom → base frame convention (author's recollection of the ROS conventions document).
  - Lab steps 5 and 6 untested.
- **F9-54** (2 boxes):
  - Nav2 grid, lattice and hybrid-A* planner plugins and their options.
  - Untested on hardware.
- **F9-55** (2 boxes):
  - OMPL planners, MoveIt's default use of OMPL, the "longest valid segment" setting.
  - Untested on hardware.
- **F9-56** (2 boxes plus Safety):
  - Nav2 server names (`bt_navigator`, `planner_server`, `controller_server`, `behavior_server`, `lifecycle_manager`).
  - `NavigateToPose`; costmap layer plugins.
  - Parameter names (`robot_radius`/`footprint`, `inflation_radius`, `cost_scaling_factor`, goal tolerances, progress checker). The mini stack's key names were chosen to resemble these, but its file format is our own.
  - Untested on hardware.
- **F9-57** (2 boxes plus Safety):
  - `move_group`, `MoveGroupInterface`, `PlanningSceneInterface`, collision/attached-object messages.
  - SRDF contents, kinematics plugin, OMPL pipeline, time-parameterisation adapter, scaling factors, Cartesian path "fraction", MoveIt Task Constructor.
  - Untested on hardware.
- **F9-58** (2 boxes plus Safety):
  - Nav2's BT navigator with BehaviorTree.CPP XML trees and its node names.
  - BehaviorTree.CPP node types, blackboard and ports, async actions with halt.
  - SMACH and FlexBE.
  - Untested on hardware.

Claims about algorithms (A* optimality proof, weighted-A* bound, RRT probabilistic completeness, RRT* radius schedule and RRT's non-optimality, PRM, RRT-Connect, pure pursuit curvature, DWA, layered costmaps, statecharts) are tagged to the original papers or textbooks above, which were not opened. They are standard results, but they remain pending until G1 closes.

## Analogies (F9 world: riding a bicycle; a body with senses and muscles)

Registered and used: SLAM = drawing the map of a dark house with a torch (F9-53).

Proposed new mappings for the Analogy Registry owner:

- **F9-52:** occupancy grid = pencil shading on squared paper (darker for "wall seen here", eraser for "beam passed through").
- **F9-54:** graph search / A* = choosing a cycling route on a town map, ranking corners by "ridden so far + crow-flies distance".
- **F9-55:** RRT = exploring an unknown park by riding towards a randomly chosen tree, always from the nearest spot already reached; shortcutting = cutting corners on the way home.
- **F9-56:** navigation stack = planning the route at home, then steering on the road around what you meet, and stopping to think when stuck.
- **F9-57:** motion planning for an arm = reaching for a cup on a high shelf; the held cup makes your hand bigger (attached object).
- **F9-58:** behaviour tree = a cyclist's habits checked every moment in priority order; state machine = the stages of a trip with rules for moving between them.

## Decisions for the owner

1. **Story characters.** These names are not in any registry I could find. Please confirm or replace them:
   - Lina appears in F9-52, F9-53, F9-57 and F9-58.
   - Ravi appears in F9-54, F9-55 and F9-56.
   - Mira (Ravi's cousin) appears in F9-55.
2. **Own simulators instead of ROS 2.** The build container has no ROS 2, so the labs use our own C++ simulators. If the kit later provides ROS 2 with Gazebo, Nav2 and MoveIt 2, the optional "untested" lab steps should be written and run then, with verified names.
3. **Forensic of F9-58 exits 0.**
   - `bt_bug` exits 0 because the faulty tree reports SUCCESS.
   - `fetch_bt` also exits 0 although run 2 does not deliver the cup, because it checks only root statuses. The chapter uses this as a teaching point (lab "Expected observations").
   - If you prefer the lab program itself to check the cup, change line 14 of `fetch_bt.cpp`. That would make its exit code 1.
4. **Exam P.** The course card's practical "configure navigation for a new map" is rehearsed only as the F9-56 mini-project on the mini stack. A real exam on Nav2 needs a ROS 2 installation and verified parameter names.
5. **Word counts.** Prose without tables is about 4,800–6,300 words per chapter. F9-57 and F9-58 are slightly under the 5,000-word L4 target if tables are excluded, and above it if tables are included.
6. **Prerequisite chapters.** The `prereqs` lines name individual chapters of RB301 (F9-23 to F9-27), RB302 (F9-31, F9-34, F9-36), RB303 (F9-43) and RB304 (F9-49) where the text relies on them. Please check that these chapters, once written, cover what is assumed.
7. **Shared code.** `labs/F9-55`, `F9-56` and `F9-57` include headers from `labs/F9-52` and `labs/F9-54` by relative path (`../F9-54/planner.hpp`, `../F9-52/house.hpp`). Moving lab folders would break them.
