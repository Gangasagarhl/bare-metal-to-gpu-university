// driver_states.cpp - F10-27 Listing 5: the life cycle of a HAL driver object, modelled on
// the pattern the ChibiOS HAL documentation describes (a driver object per peripheral,
// a configuration structure, start/stop calls and a state machine that rejects calls made
// in the wrong state). The names (SerialDriver, State, start, stop, write) are OURS; the
// real ChibiOS names are in the chapter's unverified box.
#include <cstdio>
#include <string>

enum class State { Uninit, Stop, Ready };

const char* name(State s)
{
    switch (s) {
    case State::Uninit: return "UNINIT";
    case State::Stop: return "STOP";
    case State::Ready: return "READY";
    }
    return "?";
}

struct SerialConfig {
    unsigned baud;
};

class SerialDriver {
public:
    explicit SerialDriver(const char* id) : id_(id) {}

    void init()                                  // once, at system start
    {
        check(state_ == State::Uninit, "init");
        state_ = State::Stop;                     // object valid, peripheral clock still off
        report("init");
    }
    void start(const SerialConfig& cfg)         // power the peripheral and apply the config
    {
        check(state_ == State::Stop || state_ == State::Ready, "start");
        baud_ = cfg.baud;
        state_ = State::Ready;
        report("start");
    }
    void stop()                                 // release it (clock off, pins idle)
    {
        check(state_ == State::Stop || state_ == State::Ready, "stop");
        state_ = State::Stop;
        report("stop");
    }
    bool write(const std::string& text)
    {
        if (!check(state_ == State::Ready, "write")) {
            return false;
        }
        sent_ += text.size();
        std::printf("  %s: write %zu bytes at %u baud\n", id_, text.size(), baud_);
        return true;
    }
    unsigned errors() const { return errors_; }

private:
    bool check(bool ok, const char* call)
    {
        if (!ok) {
            ++errors_;
            std::printf("  %s: ASSERT %s() called in state %s\n", id_, call, name(state_));
        }
        return ok;
    }
    void report(const char* call) const
    {
        std::printf("  %s: %s() -> state %s\n", id_, call, name(state_));
    }

    const char* id_;
    State state_ = State::Uninit;
    unsigned baud_ = 0;
    std::size_t sent_ = 0;
    unsigned errors_ = 0;
};

int main()
{
    std::puts("Correct order: init, start, write, stop");
    SerialDriver telem("SD_TELEM");
    telem.init();
    telem.start(SerialConfig{57600});
    telem.write("hello");
    telem.stop();

    std::puts("Wrong order: write before start, then start twice with new settings");
    SerialDriver gps("SD_GPS");
    gps.init();
    gps.write("$");
    gps.start(SerialConfig{9600});
    gps.start(SerialConfig{115200});             // allowed: a restart applies a new config
    gps.write("$");

    std::printf("errors caught: SD_TELEM %u, SD_GPS %u\n", telem.errors(), gps.errors());
    return 0;
}
