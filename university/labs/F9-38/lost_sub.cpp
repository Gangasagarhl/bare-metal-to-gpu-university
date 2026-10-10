// lost_sub.cpp - forensic evidence for F9-38: identical to talker_listener.cpp except Listener.
#include <memory>
#include <string>
#include "mini_rclcpp.hpp"

struct StringMsg { std::string data; };

class Talker : public mini::Node {
public:
    Talker() : Node("talker")
    {
        publisher_ = create_publisher<StringMsg>("chatter", 10);
        timer_ = create_wall_timer(500, [this] { onTimer(); });
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
        auto subscription = create_subscription<StringMsg>(
            "chatter", 10, [this](const StringMsg& msg) { log("INFO", "I heard '" + msg.data + "'"); });
        log("INFO", "subscribed to chatter");
    }
};

int main()
{
    auto talker = std::make_shared<Talker>();
    auto listener = std::make_shared<Listener>();
    mini::spin_for(2000);
    return 0;
}
