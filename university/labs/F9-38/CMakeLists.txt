# CMakeLists.txt for talker_ros2.cxx - FROM MEMORY, UNVERIFIED, UNTESTED (no ROS 2 in the build).
# Check against the ROS 2 documentation's C++ publisher tutorial and the ament_cmake guide.
cmake_minimum_required(VERSION 3.8)
project(uni_talker CXX)
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
add_executable(talker talker_ros2.cxx)
target_compile_features(talker PUBLIC cxx_std_17)
ament_target_dependencies(talker rclcpp std_msgs)
install(TARGETS talker DESTINATION lib/${PROJECT_NAME})
ament_package()
