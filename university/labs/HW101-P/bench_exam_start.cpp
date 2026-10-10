// HW101 practical exam (P): "predict, build and measure a small circuit", simulator version.
// STARTING FILE handed to the candidate. Complete the five functions marked TODO; do not change
// anything else. Build and run with the course command (one line):
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined bench_exam_start.cpp -o bench_exam_start
//   ./bench_exam_start < bench_exam_start.in
// As handed out, every TODO returns a placeholder, so the output is wrong and the self-check
// at the end reports failures (exit code 1). When all five are right it reports 0 failures.
//
// The circuit: battery -> series resistor -> LED -> back (the LED channel), and a second
// loop battery -> timer resistor -> capacitor (the RC timer). The course's model meter
// (F1-07) "measures" the channel: voltage mode across the resistor, current mode in series,
// resistance mode with the power off. Every number is an EXERCISE value (pretend parts).
//
// Input (bench_exam_start.in), four lines:
//   supply V, LED forward voltage V (pretend), LED maximum mA, target mA
//   the resistor values in the parts box (ohms), ended by 0
//   timer resistor ohms, timer capacitor farads, threshold volts
//   meter input resistance ohms (voltage mode), meter burden ohms (current mode)
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

// TODO 1 (F1-06): the smallest resistor that keeps the LED current at the target,
// simple model: R = (Vs - VF) / I.
double minimumOhms(double supply, double forwardVolts, double targetAmps)
{
    (void)supply; (void)forwardVolts; (void)targetAmps;  // delete this line when you use them
    return 0.0;  // TODO 1
}

// TODO 2: the smallest value in the box that is not below the minimum; -1 if the box has none.
double chooseFromBox(const std::vector<double>& box, double minimum)
{
    (void)minimum;  // delete this line when you use it
    return box.empty() ? -1.0 : box.front();  // TODO 2: this just takes the first value
}

// TODO 3 (F1-06, F1-07): the channel current with the simple LED model, with an extra series
// resistance (0 for the bare circuit, the meter's burden when the ammeter is in the loop).
double channelAmps(double supply, double forwardVolts, double ohms, double extraSeriesOhms)
{
    (void)supply; (void)forwardVolts; (void)ohms; (void)extraSeriesOhms;  // delete this line when you use them
    return 0.0;  // TODO 3
}

// TODO 4 (F1-05): the time for a capacitor charging from 0 V towards 'supply' through tau
// to reach 'target' volts: t = tau * ln(Vs / (Vs - Vx)).
double timeToReach(double tau, double supply, double target)
{
    (void)supply; (void)target;  // delete this line when you use them
    return tau;  // TODO 4: wrong on purpose
}

// TODO 5 (F1-05, F1-07): one time step of the capacitor voltage. The resistor feeds
// (supply - vc) / ohms; a voltmeter across the capacitor (meterOhms > 0) draws vc / meterOhms.
double stepCapacitor(double vc, double supply, double ohms, double farads, double meterOhms, double dt)
{
    (void)supply; (void)ohms; (void)farads; (void)meterOhms; (void)dt;  // delete this line when you use them
    return vc;  // TODO 5: the capacitor never charges
}

// Given: parallel combination (F1-04).
static double parallel(double a, double b) { return a * b / (a + b); }

// Given: simulate the RC loop in 1 ms steps; reports Vc at t = tau and the first crossing of the threshold.
struct RcRun { double vcAtTau; double crossing; double finalVolts; };
static RcRun simulateRc(double supply, double ohms, double farads, double threshold, double meterOhms)
{
    const double tau = ohms * farads;
    const double dt = 0.001;
    const long steps = static_cast<long>(std::lround(8.0 * tau / dt));  // 8 tau: as good as settled
    RcRun r{0.0, -1.0, 0.0};
    double vc = 0.0;
    for (long k = 1; k <= steps; ++k) {
        vc = stepCapacitor(vc, supply, ohms, farads, meterOhms, dt);
        const double t = k * dt;
        if (r.crossing < 0.0 && vc >= threshold) {
            r.crossing = t;
        }
        if (k == std::lround(tau / dt)) {
            r.vcAtTau = vc;
        }
    }
    r.finalVolts = vc;
    return r;
}

int main()
{
    double supply = 0.0, forwardVolts = 0.0, maxMilliamps = 0.0, targetMilliamps = 0.0;
    std::vector<double> box;
    double timerOhms = 0.0, timerFarads = 0.0, threshold = 0.0, meterInput = 0.0, meterBurden = 0.0;
    if (!(std::cin >> supply >> forwardVolts >> maxMilliamps >> targetMilliamps)) {
        std::cout << "bad input line 1\n";
        return 2;
    }
    for (double r = 0.0; std::cin >> r && r > 0.0;) {
        box.push_back(r);
    }
    if (!(std::cin >> timerOhms >> timerFarads >> threshold >> meterInput >> meterBurden)) {
        std::cout << "bad input line 3 or 4\n";
        return 2;
    }
    int failures = 0;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "All values are exercise values (pretend parts), not datasheet numbers.\n";

    // ---- Part 1: predict the LED channel
    std::cout << "LED channel: supply " << supply << " V, LED VF " << forwardVolts << " V, target "
              << std::setprecision(1) << targetMilliamps << " mA, maximum " << maxMilliamps << " mA\n";
    const double minimum = minimumOhms(supply, forwardVolts, targetMilliamps / 1000.0);
    const double chosen = chooseFromBox(box, minimum);
    std::cout << "  minimum resistor " << minimum << " ohms; chosen from the box: ";
    if (chosen < 0.0) {
        std::cout << "none large enough\n";
        return 1;
    }
    std::cout << std::setprecision(0) << chosen << " ohms\n" << std::setprecision(3);
    const double amps = channelAmps(supply, forwardVolts, chosen, 0.0);
    const double acrossResistor = amps * chosen;
    std::cout << "  predicted: I = " << amps * 1000.0 << " mA, V across R = " << acrossResistor
              << " V, P resistor = " << std::setprecision(4) << acrossResistor * amps << " W, P LED = "
              << forwardVolts * amps << " W\n" << std::setprecision(3);
    if (amps * 1000.0 > maxMilliamps) {
        std::cout << "  ABOVE the LED maximum: not allowed\n";
        ++failures;
    }

    // ---- Part 2: "measure" the channel with the model meter
    const double loadedOhms = parallel(chosen, meterInput);
    const double ampsWithVoltmeter = channelAmps(supply, forwardVolts, loadedOhms, 0.0);
    const double ampsWithAmmeter = channelAmps(supply, forwardVolts, chosen, meterBurden);
    std::cout << "  voltage mode across R (meter " << std::setprecision(0) << meterInput << " ohms in parallel): "
              << std::setprecision(3) << ampsWithVoltmeter * loadedOhms << " V; the loop current changes by "
              << std::setprecision(4) << (ampsWithVoltmeter - amps) / amps * 100.0 << " %\n" << std::setprecision(3);
    std::cout << "  current mode in series (burden " << meterBurden << " ohms): " << ampsWithAmmeter * 1000.0
              << " mA, " << std::setprecision(2) << (ampsWithAmmeter - amps) / amps * 100.0 << " % change\n"
              << std::setprecision(3);
    std::cout << "  resistance mode, power off, part out of the circuit: " << std::setprecision(1) << chosen
              << " ohms\n" << std::setprecision(3);
    if (!(ampsWithAmmeter < amps)) {
        ++failures;
    }

    // ---- Part 3: the RC timer, predicted and simulated
    const double tau = timerOhms * timerFarads;
    const double predictedVcAtTau = supply * (1.0 - std::exp(-1.0));
    const double predictedCrossing = timeToReach(tau, supply, threshold);
    std::cout << "RC timer: R = " << std::setprecision(0) << timerOhms << " ohms, C = " << std::setprecision(1)
              << timerFarads * 1e6 << " uF, tau = " << std::setprecision(3) << tau << " s\n";
    std::cout << "  predicted: Vc at 1 tau = " << predictedVcAtTau << " V; reaches " << threshold << " V at t = "
              << predictedCrossing << " s\n";
    const RcRun bare = simulateRc(supply, timerOhms, timerFarads, threshold, 0.0);
    std::cout << "  simulated (1 ms steps): Vc at 1 tau = " << bare.vcAtTau << " V; crosses " << threshold
              << " V at t = " << bare.crossing << " s\n";
    const RcRun metered = simulateRc(supply, timerOhms, timerFarads, threshold, meterInput);
    const double predictedFinal = supply * meterInput / (timerOhms + meterInput);
    std::cout << "  with the voltmeter across C: settles at " << metered.finalVolts << " V (divider prediction "
              << predictedFinal << " V); crosses " << threshold << " V at t = " << metered.crossing << " s\n";
    if (std::fabs(bare.vcAtTau - predictedVcAtTau) > 0.01) {
        ++failures;
    }
    if (bare.crossing < 0.0 || std::fabs(bare.crossing - predictedCrossing) > 0.01) {
        ++failures;
    }
    if (std::fabs(metered.finalVolts - predictedFinal) > 0.01) {
        ++failures;
    }
    std::cout << "self-check: " << failures << " failures\n";
    return failures > 0 ? 1 : 0;
}
