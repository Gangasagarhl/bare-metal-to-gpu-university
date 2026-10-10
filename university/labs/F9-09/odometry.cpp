// F9-09 Listing 2: wheel odometry for a differential-drive robot. Reads a recorded
// encoder log (t, cumulative left and right counts) and integrates the pose x, y, heading.
// The three parameters are the robot's calibration; here they are the pretend values
// used by the simulator that wrote the log.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <numbers>
#include <sstream>
#include <string>

struct Pose
{
    double x = 0.0;
    double y = 0.0;
    double theta = 0.0;  // rad, counter-clockwise from the x axis
};

// One odometry step from the distances rolled by the two wheels since the last step.
Pose update(Pose p, double dsL, double dsR, double track)
{
    const double ds = (dsR + dsL) / 2.0;
    const double dtheta = (dsR - dsL) / track;
    const double mid = p.theta + dtheta / 2.0;  // heading halfway through the step
    p.x += ds * std::cos(mid);
    p.y += ds * std::sin(mid);
    p.theta += dtheta;
    return p;
}

int main()
{
    const double wheelDiameter = 0.070;  // m
    const double track = 0.160;          // m
    const double cpr = 1440.0;           // counts per wheel revolution
    const double mPerCount = std::numbers::pi * wheelDiameter / cpr;

    Pose pose;
    long prevL = 0;
    long prevR = 0;
    bool first = true;
    int line = 0;
    std::string text;
    while (std::getline(std::cin, text)) {
        if (text.empty() || text[0] == '#') {
            continue;
        }
        std::istringstream in(text);
        double t = 0.0;
        long cL = 0;
        long cR = 0;
        if (!(in >> t >> cL >> cR)) {
            std::printf("bad line: %s\n", text.c_str());
            return 1;
        }
        if (!first) {
            pose = update(pose, (cL - prevL) * mPerCount, (cR - prevR) * mPerCount, track);
        }
        first = false;
        prevL = cL;
        prevR = cR;
        if (line % 20 == 0) {
            std::printf("t=%5.1f s  counts %6ld %6ld  x=%7.4f m  y=%7.4f m  heading=%8.3f deg\n", t,
                        cL, cR, pose.x, pose.y, pose.theta * 180.0 / std::numbers::pi);
        }
        ++line;
    }
    std::printf("end: x=%.4f m  y=%.4f m  heading=%.3f deg  (true end: 0, 0, 360)\n", pose.x,
                pose.y, pose.theta * 180.0 / std::numbers::pi);
    return 0;
}
