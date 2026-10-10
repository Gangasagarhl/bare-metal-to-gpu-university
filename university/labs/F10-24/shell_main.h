// shell_main.h - a tiny command shell for the mini runtime: start, stop and query the
// tilt_guard module, run the simulation, read topics and parameters. Commands come
// from standard input. Include a TiltGuard class before this header.
#pragma once

#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "mini_px4.h"

inline int runShell()
{
    mini_px4::System sys;
    sys.defineParam(TiltGuard::kParam, 30.0f);
    std::unique_ptr<TiltGuard> guard;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::printf("nsh-model> %s\n", line.c_str());
        std::istringstream in(line);
        std::string cmd;
        std::string arg;
        in >> cmd >> arg;
        if (cmd == "tilt_guard" && arg == "start") {
            if (guard) {
                std::printf("tilt_guard: already running\n");
            } else {
                guard = std::make_unique<TiltGuard>(sys);
                std::printf("tilt_guard: started\n");
            }
        } else if (cmd == "tilt_guard" && arg == "stop") {
            guard.reset();
            std::printf("tilt_guard: stopped\n");
        } else if (cmd == "tilt_guard" && arg == "status") {
            if (guard) {
                guard->printStatus();
            } else {
                std::printf("tilt_guard: not running\n");
            }
        } else if (cmd == "sim" && arg == "run") {
            double s = 0.0;
            in >> s;
            sys.runFor(s);
            std::printf("simulated time now %.3f s\n", static_cast<double>(sys.nowUs) * 1e-6);
        } else if (cmd == "listener" && arg == "tilt_status") {
            uorb_lite::Subscription<mini_px4::TiltStatus, 1> sub(sys.tiltStatus);
            mini_px4::TiltStatus m{};
            if (sub.update(m)) {
                std::printf("tilt_status: timestamp %llu us, tilt %.2f deg, max %.1f deg, exceeded %s, "
                            "count %u\n",
                            static_cast<unsigned long long>(m.timestampUs), static_cast<double>(m.tiltDeg),
                            static_cast<double>(m.maxTiltDeg), m.exceeded ? "yes" : "no", m.exceedCount);
            } else {
                std::printf("tilt_status: never published\n");
            }
        } else if (cmd == "param" && arg == "set") {
            std::string name;
            float v = 0.0f;
            in >> name >> v;
            std::printf("%s\n", sys.setParam(name, v) ? "ok" : "unknown parameter");
        } else if (cmd == "param" && arg == "show") {
            std::string name;
            in >> name;
            float v = 0.0f;
            if (sys.param(name, v)) {
                std::printf("%s = %.1f\n", name.c_str(), static_cast<double>(v));
            } else {
                std::printf("unknown parameter\n");
            }
        } else if (cmd == "uorb" && arg == "status") {
            sys.printTopics();
        } else {
            std::printf("unknown command\n");
        }
    }
    return 0;
}
