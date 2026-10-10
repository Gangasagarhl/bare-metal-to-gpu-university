// sitl_core.h - a tiny software-in-the-loop (SITL) setup in two processes.
// The simulator process owns the physics (a vehicle that can only move up and down)
// and the clock. The autopilot process owns the estimator and the controller.
// They talk over a local socket pair in LOCKSTEP: the simulator sends one sensor
// message, waits for the actuator reply, and only then advances time by one step.
// This is the course's own model of the idea; it is not PX4 or any simulator.
#pragma once

#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace sitl {

struct SensorMsg
{
    std::uint64_t timeUs;   // simulated time of the sample, microseconds
    float baroAltM;         // barometric altitude, with a small deterministic noise
    float accelUpMps2;      // specific force along "up" (reads +9.81 when standing still)
};

struct ActuatorMsg
{
    std::uint64_t timeUs;   // the sample this command answers
    float thrust;           // 0 .. 1
};

constexpr double kStepS = 0.004;       // 250 Hz physics and control
constexpr double kG = 9.81;
constexpr double kMassKg = 1.5;        // model values, not a real vehicle
constexpr double kMaxThrustN = 30.0;
constexpr int kSteps = 2500;           // 10 s of simulated time

// --------------------------------------------------------------------------- simulator
inline int runSimulator(int fd, bool timeInMillisBug)
{
    double z = 0.0;
    double vz = 0.0;
    std::uint32_t lcg = 12345;   // fixed seed: the run is repeatable
    double lastAccel = 0.0;
    int replies = 0;
    for (int k = 0; k < kSteps; ++k) {
        const std::uint64_t tUs = static_cast<std::uint64_t>(k) * 4000;
        lcg = lcg * 1664525u + 1013904223u;
        const double noise = (static_cast<double>(lcg >> 8) / 16777216.0 - 0.5) * 0.02;   // +-1 cm
        SensorMsg s{timeInMillisBug ? tUs / 1000 : tUs, static_cast<float>(z + noise),
                    static_cast<float>(lastAccel + kG)};
        if (send(fd, &s, sizeof s, 0) != static_cast<ssize_t>(sizeof s)) {
            return 1;
        }
        ActuatorMsg a{};
        if (recv(fd, &a, sizeof a, 0) != static_cast<ssize_t>(sizeof a)) {   // lockstep: wait
            return 1;
        }
        ++replies;
        const double thrustN = std::fmin(std::fmax(a.thrust, 0.0f), 1.0f) * kMaxThrustN;
        double acc = thrustN / kMassKg - kG;
        if (z <= 0.0 && acc < 0.0) {   // standing on the ground
            acc = 0.0;
            vz = 0.0;
        }
        vz += acc * kStepS;
        z = std::fmax(0.0, z + vz * kStepS);
        lastAccel = acc;
    }
    ActuatorMsg bye{};   // wait until the autopilot has printed everything (keeps the output order fixed)
    if (recv(fd, &bye, sizeof bye, 0) != static_cast<ssize_t>(sizeof bye)) {
        return 1;
    }
    std::printf("simulator: %d steps sent, %d actuator replies received (lockstep)\n", kSteps, replies);
    return 0;
}

// --------------------------------------------------------------------------- autopilot
inline int runAutopilot(int fd)
{
    double zEst = 0.0;
    double vzEst = 0.0;
    double lastBaro = 0.0;
    double integ = 0.0;
    std::uint64_t lastT = 0;
    bool first = true;
    std::printf("autopilot: t_s   setpoint_m  alt_est_m  vz_est_mps  thrust  [dt_s]\n");
    for (int k = 0; k < kSteps; ++k) {
        SensorMsg s{};
        if (recv(fd, &s, sizeof s, 0) != static_cast<ssize_t>(sizeof s)) {
            return 1;
        }
        // dt comes from the message timestamps, as a flight stack's would.
        const double dt = first ? kStepS : static_cast<double>(s.timeUs - lastT) * 1e-6;
        // Estimator: velocity from the difference of baro altitudes, low-pass filtered.
        const double vMeas = first ? 0.0 : (s.baroAltM - lastBaro) / dt;
        vzEst += 0.1 * (vMeas - vzEst);
        zEst += 0.2 * (s.baroAltM - zEst);
        lastBaro = s.baroAltM;
        lastT = s.timeUs;
        first = false;
        // Controller: altitude error -> velocity setpoint -> thrust (with hover feed-forward).
        const double tS = k * kStepS;
        const double zSp = tS < 1.0 ? 0.0 : 2.0;
        const double vSp = std::fmax(-1.0, std::fmin(1.0, 1.2 * (zSp - zEst)));
        const double vErr = vSp - vzEst;
        integ = std::fmax(-0.2, std::fmin(0.2, integ + 0.05 * vErr * dt));
        const double hover = kMassKg * kG / kMaxThrustN;
        const double thrust = std::fmax(0.0, std::fmin(1.0, hover + 0.25 * vErr + integ));
        ActuatorMsg a{s.timeUs, static_cast<float>(thrust)};
        if (send(fd, &a, sizeof a, 0) != static_cast<ssize_t>(sizeof a)) {
            return 1;
        }
        if (k % 125 == 0) {   // every 0.5 s of simulated time
            std::printf("autopilot: %5.2f  %10.2f  %9.3f  %10.3f  %6.3f  [%.6f]\n", tS, zSp, zEst, vzEst,
                        thrust, dt);
        }
    }
    std::fflush(stdout);
    const ActuatorMsg bye{0, 0.0f};
    return send(fd, &bye, sizeof bye, 0) == static_cast<ssize_t>(sizeof bye) ? 0 : 1;
}

// Starts both processes and waits for both; returns 0 if both succeeded.
inline int runSitl(bool timeInMillisBug)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, fds) != 0) {
        std::perror("socketpair");
        return 1;
    }
    std::fflush(stdout);
    const pid_t child = fork();
    if (child < 0) {
        std::perror("fork");
        return 1;
    }
    if (child == 0) {   // the simulator process
        close(fds[0]);
        const int rc = runSimulator(fds[1], timeInMillisBug);
        std::fflush(stdout);
        _exit(rc);
    }
    close(fds[1]);     // the autopilot process
    const int rcAp = runAutopilot(fds[0]);
    std::fflush(stdout);
    int status = 0;
    waitpid(child, &status, 0);
    close(fds[0]);
    const bool simOk = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    return (rcAp == 0 && simOk) ? 0 : 1;
}

} // namespace sitl
