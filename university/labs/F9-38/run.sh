#!/usr/bin/env bash
# Extra run for F9-38:
#   talker_ros2   a real compile ATTEMPT of talker_ros2.cxx (rclcpp code written from memory).
#                 The build container has no ROS 2, so the expected, honest result is that the
#                 compiler cannot find rclcpp/rclcpp.hpp. This proves only that ROS 2 is absent.
set -u
GXX="$(g++ --version | head -n 1)"
cmd="g++ -std=c++17 -fsyntax-only -x c++ talker_ros2.cxx"
{
    echo "listing:   talker_ros2.cxx"
    echo "toolchain: $GXX"
    echo "command:   $cmd"
    echo "date:      $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "machine:   $(uname -s) $(uname -m) (cloud build container)"
} > talker_ros2.log
g++ -std=c++17 -fsyntax-only -x c++ talker_ros2.cxx > talker_ros2.out 2>&1; rc=$?
sed -i "s#$(pwd)/##g" talker_ros2.out
echo "exit code: $rc (expected non-zero: ROS 2 headers are not installed)" >> talker_ros2.log
echo "hardware:  untested: no ROS 2 installation in the build container; build with colcon in a ROS 2 workspace" >> talker_ros2.log
[ "$rc" != 0 ]   # success of this step = the compile attempt failed only because ROS 2 is absent
grep -q "rclcpp/rclcpp.hpp: No such file or directory" talker_ros2.out
