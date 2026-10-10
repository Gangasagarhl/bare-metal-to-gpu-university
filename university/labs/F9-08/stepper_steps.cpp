// F9-08 Listing 2: a two-phase stepper in full steps, and what step rate a move needs.
// Steps per revolution, lead-screw lead and microstep setting are PRETEND values;
// real ones come from the motor's and driver's datasheets.
#include <array>
#include <cstdio>

int main()
{
    // Full-step drive with one phase on at a time: the current pattern repeats every 4 steps.
    const std::array<const char*, 4> pattern{"A+ only", "B+ only", "A- only", "B- only"};
    for (int step = 0; step < 6; ++step) {
        std::printf("step %d: coil %s\n", step, pattern[static_cast<unsigned>(step % 4)]);
    }

    const int stepsPerRev = 200;     // full steps per revolution
    const int microsteps = 8;        // driver setting: microsteps per full step
    const double leadMm = 8.0;       // lead screw: mm of travel per revolution
    const double degPerStep = 360.0 / stepsPerRev;
    const double mmPerMicrostep = leadMm / (stepsPerRev * microsteps);
    std::printf("\nfull step = %.2f deg; one microstep moves the carriage %.5f mm\n", degPerStep,
                mmPerMicrostep);

    std::printf("\ncarriage speed (mm/s)  revolutions/s  microstep pulses/s\n");
    for (double speed : {5.0, 20.0, 50.0, 100.0}) {
        const double revPerSec = speed / leadMm;
        const double pulsesPerSec = revPerSec * stepsPerRev * microsteps;
        std::printf("%21.1f %14.3f %19.0f\n", speed, revPerSec, pulsesPerSec);
    }
    return 0;
}
