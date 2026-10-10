// talker_listener.cpp - two nodes in the shape of an rclcpp program, built on mini_rclcpp.hpp (F9-38).
#include <memory>
#include <string>
#include "mini_rclcpp.hpp"

struct StringMsg { std::string data; };    // stands in for a ROS 2 string message type

class Talker : public mini::Node {
public:
    Talker() : Node("talker")
    {
        publisher_ = create_publisher<StringMsg>("chatter", 10);
        timer_ = create_wall_timer(500, [this] { onTimer(); });    // every 500 ms
    }
private:
    void onTimer()
    {
        StringMsg msg{"hello " + std::to_string(count_++)};
        log("INFO", "publishing '" + msg.data + "' to " +
            std::to_string(publisher_->get_subscription_count()) + " subscriber(s)");
        publisher_->publish(msg);
    }
    std::shared_ptr<mini::Publisher<StringMsg>> publisher_;
    std::shared_ptr<mini::TimerBase> timer_;
    int count_ = 0;
};

class Listener : public mini::Node {
public:
    Listener() : Node("listener")
    {
        subscription_ = create_subscription<StringMsg>(
            "chatter", 10, [this](const StringMsg& msg) { log("INFO", "I heard '" + msg.data + "'"); });
    }
private:
    std::shared_ptr<mini::Subscription<StringMsg>> subscription_;   // keep it alive!
};

int main()
{
    auto talker = std::make_shared<Talker>();
    auto listener = std::make_shared<Listener>();
    mini::spin_for(2000);                  // 2 s of simulated time
    listener.reset();                      // the listener node is destroyed
    talker->log("INFO", "listener node destroyed");
    mini::spin_for(1000);
    return 0;
}
