# robot.launch.py - a ROS 2 Python launch file, written FROM MEMORY for chapter F9-42.
# UNVERIFIED and UNTESTED: ROS 2 and its launch packages are not installed in the build container.
# Only Python's own syntax check was run on this file. Check every module, class and argument
# name against the ROS 2 documentation ("Launch" tutorials, "Using parameters in a class",
# "Managed nodes") for the distribution you install.
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    odom = Node(
        package='uni_robot',
        executable='odom_node',
        name='odom',
        namespace='robot1',
        parameters=['params.yaml'],
        remappings=[('wheel_ticks', 'base/wheel_ticks')],
        output='screen',
    )
    teleop = Node(
        package='uni_robot',
        executable='teleop_node',
        name='teleop',
        namespace='robot1',
    )
    return LaunchDescription([odom, teleop])
