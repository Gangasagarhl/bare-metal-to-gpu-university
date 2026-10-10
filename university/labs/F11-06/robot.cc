// robot.cc - F11-06 Listing 1: the chain of trust of a robot with three processors, modelled
// on the host: the main computer (MC, runs the robot software), the motion controller (FC,
// runs the control loops) and the motor driver (MD). Each has a boot ROM with a fused key hash,
// a security counter and a debug-lock fuse; each verifies its own image with fwimage.h. The
// main computer enables the motors only if every processor reports a verified boot and the
// versions are in the signed compatibility list.
//
//   robot demo       the design decisions, one scenario each
//   robot forensic   the evidence pack of the forensic lab
#include <algorithm>
#include <cstring>
#include <iostream>
#include <map>

#include "fwimage.h"

namespace {

using fw::Bytes;

struct Processor {
    std::string name;
    std::vector<EVP_PKEY*> rom_keys;   // keys whose hashes are fused into the boot ROM's OTP
    uint32_t counter = 1;              // anti-rollback fuse value
    bool debug_locked = true;          // debug-lock fuse
    Bytes primary, secondary;          // installed image, previous image (kept for revert)
    std::string log;

    // Secure boot of one processor. Returns the version string that runs, or "" (stays in
    // its boot ROM's recovery mode: motors cannot be driven).
    std::string boot()
    {
        const fw::Verdict v = fw::validate(primary, rom_keys, counter);
        log += "    [" + name + "] primary: " + v.reason + "\n";
        if (v.ok) { return version_of(primary); }
        if (!secondary.empty()) {
            const fw::Verdict w = fw::validate(secondary, rom_keys, counter);
            log += "    [" + name + "] previous image: " + w.reason + "\n";
            if (w.ok) { std::swap(primary, secondary); return version_of(primary); }
        }
        log += "    [" + name + "] no bootable image: recovery mode\n";
        return "";
    }

    static std::string version_of(const Bytes& img)
    {
        const auto p = fw::parse(img);
        return std::to_string(p->header.major) + "." + std::to_string(p->header.minor);
    }
};

// The signed compatibility list: which version combinations were tested together.
struct Manifest {
    std::vector<std::map<std::string, std::string>> tested;
    bool allows(const std::map<std::string, std::string>& running) const
    {
        return std::find(tested.begin(), tested.end(), running) != tested.end();
    }
};

struct Robot {
    std::vector<Processor> cpus;
    Manifest manifest;

    void power_on()
    {
        std::map<std::string, std::string> running;
        bool all = true;
        for (Processor& p : cpus) {
            p.log.clear();
            const std::string v = p.boot();
            std::cout << p.log;
            if (v.empty()) { all = false; }
            running[p.name] = v.empty() ? "none" : v;
        }
        std::cout << "    running:";
        for (const auto& [n, v] : running) { std::cout << " " << n << " " << v; }
        if (!all) {
            std::cout << "\n    MC decision: a processor is in recovery mode -> motors stay DISABLED (safe state)\n";
        } else if (!manifest.allows(running)) {
            std::cout << "\n    MC decision: combination not in the signed compatibility list -> motors stay DISABLED\n";
        } else {
            std::cout << "\n    MC decision: all verified, combination tested -> motors may be ENABLED (arming still needs the operator)\n";
        }
    }

    Processor& cpu(const std::string& n)
    {
        return *std::find_if(cpus.begin(), cpus.end(), [&](const Processor& p) { return p.name == n; });
    }
};

struct Keys {
    fw::Pkey mc = fw::load_or_make_key("robot_mc"), fc = fw::load_or_make_key("robot_fc"),
             md = fw::load_or_make_key("robot_md"), attacker = fw::load_or_make_key("attacker");
};

Robot factory_robot(const Keys& k)
{
    Robot r;
    for (const auto& [name, key] : {std::pair{"MC", k.mc.get()}, std::pair{"FC", k.fc.get()}, std::pair{"MD", k.md.get()}}) {
        Processor p;
        p.name = name;
        p.rom_keys = {key};
        r.cpus.push_back(p);
    }
    r.cpus[0].primary = fw::build_image("main computer software", 4, 2, 1, k.mc.get());
    r.cpus[1].primary = fw::build_image("motion controller", 2, 7, 1, k.fc.get());
    r.cpus[2].primary = fw::build_image("motor driver", 1, 3, 1, k.md.get());
    r.manifest.tested = {{{"MC", "4.2"}, {"FC", "2.7"}, {"MD", "1.3"}},
                         {{"MC", "4.3"}, {"FC", "2.8"}, {"MD", "1.3"}}};
    return r;
}

// An update of one processor; "burn_early" raises the anti-rollback fuse at install time,
// before the new image has proved itself (the policy the forensic lab is about).
void update(Processor& p, const Bytes& img, bool burn_early)
{
    const fw::Verdict v = fw::validate(img, p.rom_keys, p.counter);
    std::cout << "    [" << p.name << "] update candidate: " << v.reason << "\n";
    if (!v.ok) { return; }
    p.secondary = p.primary;
    p.primary = img;
    if (burn_early) {
        p.counter = fw::parse(img)->header.security_counter;
        std::cout << "    [" << p.name << "] anti-rollback fuse raised to " << p.counter << " at install time\n";
    }
}

int demo()
{
    const Keys k;
    std::cout << "1. Factory state\n";
    Robot r = factory_robot(k);
    r.power_on();

    std::cout << "\n2. The motion controller's image is modified in flash (a bit flipped by an attacker or by wear)\n";
    Robot r2 = factory_robot(k);
    r2.cpu("FC").primary[fw::kHeaderSize + 3] ^= 0x20;
    r2.power_on();

    std::cout << "\n3. The motor driver is updated alone to 1.4 (signed, valid) but 1.4 was never tested with this set\n";
    Robot r3 = factory_robot(k);
    update(r3.cpu("MD"), fw::build_image("motor driver", 1, 4, 1, k.md.get()), false);
    r3.power_on();

    std::cout << "\n4. One key per processor: the main computer's key cannot sign motor-driver firmware\n";
    Robot r4 = factory_robot(k);
    update(r4.cpu("MD"), fw::build_image("motor driver from the MC build server", 1, 3, 1, k.mc.get()), false);

    std::cout << "\n5. Debug port left unlocked on the motor driver (fuse not blown at the factory)\n";
    Robot r5 = factory_robot(k);
    r5.cpu("MD").debug_locked = false;
    r5.power_on();
    const bool open = !r5.cpu("MD").debug_locked;
    std::cout << "    after boot, a probe on the MD debug port: "
              << (open ? "connects; can halt the CPU and write RAM: the code that runs is no longer the code that was verified"
                       : "refused") << "\n";
    std::cout << "    end-of-line check (factory test): MD debug lock = " << (open ? "OPEN -> unit must not ship" : "locked") << "\n";

    std::cout << "\n6. Coordinated update 4.3 / 2.8 / 1.3 (the second tested set), installed and confirmed\n";
    Robot r6 = factory_robot(k);
    update(r6.cpu("MC"), fw::build_image("main computer software", 4, 3, 2, k.mc.get()), false);
    update(r6.cpu("FC"), fw::build_image("motion controller", 2, 8, 2, k.fc.get()), false);
    r6.power_on();
    return 0;
}

int forensic()
{
    const Keys k;
    Robot r = factory_robot(k);
    Processor& fc = r.cpu("FC");
    std::cout << "EVIDENCE 1 - service log of drone D-0311 (motion controller update 2.7 -> 2.9)\n";
    update(fc, fw::build_image("motion controller 2.9 (ESC protocol change)", 2, 9, 2, k.fc.get()), true);
    std::cout << "    [FC] test boot of 2.9: motor drivers answer with an unknown protocol, self-test FAILED, image not confirmed\n";
    std::cout << "    [FC] reverting to the previous image\n";
    // the revert puts 2.7 back in the primary slot
    std::swap(fc.primary, fc.secondary);
    std::cout << "EVIDENCE 2 - next power-on\n";
    r.power_on();
    std::cout << "EVIDENCE 3 - fuse read-out of the motion controller\n";
    std::cout << "    security counter fuse = " << fc.counter << "\n";
    for (const Bytes* img : {&fc.primary, &fc.secondary}) {
        const auto p = fw::parse(*img);
        std::cout << "    " << (img == &fc.primary ? "primary slot:  " : "previous slot: ") << "image "
                  << Processor::version_of(*img) << " carries security counter " << p->header.security_counter << "\n";
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "forensic") == 0) {
        return forensic();
    }
    return demo();
}
