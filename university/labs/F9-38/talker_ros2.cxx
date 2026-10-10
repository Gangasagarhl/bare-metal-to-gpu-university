// talker_ros2.cxx - the same Talker written against the real rclcpp API, FROM MEMORY.
// UNVERIFIED and UNTESTED in this build: ROS 2 is not installed in the build container.
// Check every name against the ROS 2 documentation (rclcpp API, "Writing a simple publisher
// and subscriber (C++)") for the distribution you install, then build it with colcon.
#include <chrono>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class Talker : public rclcpp::Node
{
public:
    Talker() : Node("talker")
    {
        publisher_ = create_publisher<std_msgs::msg::String>("chatter", 10);
        timer_ = create_wall_timer(500ms, [this] { onTimer(); });
    }
private:
    void onTimer()
    {
        std_msgs::msg::String msg;
        msg.data = "hello " + std::to_string(count_++);
        RCLCPP_INFO(get_logger(), "publishing '%s'", msg.data.c_str());
        publisher_->publish(msg);
    }
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int count_ = 0;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<Talker>());
    rclcpp::shutdown();
    return 0;
}
