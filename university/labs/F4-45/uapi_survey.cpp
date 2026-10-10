// uapi_survey.cpp - DR405 F4-45: what the Linux UAPI headers installed in this container
// (linux-libc-dev, under /usr/include/drm) reveal about the open GPU kernel drivers, and
// which GPU firmware files the container carries under /lib/firmware.
// A header's presence proves only that this kernel version exports that driver's interface.
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main()
{
    const fs::path drm = "/usr/include/drm";
    std::vector<fs::path> headers;
    for (const auto& e : fs::directory_iterator(drm)) {
        const std::string n = e.path().filename().string();
        if (n.ends_with("_drm.h") || n.ends_with("_accel.h")) headers.push_back(e.path());
    }
    std::sort(headers.begin(), headers.end());
    std::printf("%-22s %7s  %s\n", "header", "ioctls", "submission-like ioctls (name contains SUBMIT, EXEC or _CS)");
    for (const auto& h : headers) {
        std::ifstream in(h);
        std::string line, subs;
        int n = 0;
        while (std::getline(in, line)) {
            if (line.rfind("#define DRM_IOCTL_", 0) != 0) continue;
            ++n;
            const std::string name = line.substr(8, line.find_first_of(" \t", 8) - 8);
            if (name.find("SUBMIT") != std::string::npos || name.find("EXEC") != std::string::npos ||
                name.ends_with("_CS"))
                subs += (subs.empty() ? "" : " ") + name.substr(10);
        }
        std::printf("%-22s %7d  %s\n", h.filename().c_str(), n, subs.empty() ? "-" : subs.c_str());
    }
    std::printf("\nheaders for the drivers named in curriculum 16.1 (present in this kernel's UAPI?)\n");
    const char* want[] = {"i915", "xe", "amdgpu", "radeon", "nouveau", "lima", "panfrost", "panthor",
                          "msm", "vc4", "v3d", "pvr", "etnaviv", "asahi"};
    for (const char* w : want)
        std::printf("  %-9s %s\n", w, fs::exists(drm / (std::string(w) + "_drm.h")) ? "present" : "ABSENT");

    std::printf("\nGPU firmware files under /lib/firmware\n");
    std::map<std::string, std::uintmax_t> fw;
    if (fs::exists("/lib/firmware"))
        for (const auto& e : fs::recursive_directory_iterator("/lib/firmware"))
            if (e.is_regular_file()) fw[e.path().string()] = e.file_size();
    for (const auto& [p, s] : fw) std::printf("  %-50s %10ju bytes\n", p.c_str(), s);
    std::printf("  (%zu files)\n", fw.size());
    return 0;
}
