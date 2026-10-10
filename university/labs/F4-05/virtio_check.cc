// virtio_check.cc - DR301 F4-05: compare virtio.h (written from the author's memory of the
// OASIS VIRTIO specification) with the values Linux's UAPI headers give (read from stdin,
// as printed by linux_values.c). Agreement of two independent sources is evidence, not
// proof; a disagreement would prove one of them wrong.
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include "virtio.h"

int main()
{
    std::map<std::string, unsigned long> lx;
    std::string name;
    unsigned long v;
    while (std::cin >> name >> v) lx[name] = v;
    const std::map<std::string, unsigned long> ours = {
        {"sizeof_common_cfg", sizeof(vio::CommonCfg)},
        {"off_device_feature_select", offsetof(vio::CommonCfg, device_feature_select)},
        {"off_device_feature", offsetof(vio::CommonCfg, device_feature)},
        {"off_driver_feature_select", offsetof(vio::CommonCfg, driver_feature_select)},
        {"off_driver_feature", offsetof(vio::CommonCfg, driver_feature)},
        {"off_num_queues", offsetof(vio::CommonCfg, num_queues)},
        {"off_device_status", offsetof(vio::CommonCfg, device_status)},
        {"off_queue_select", offsetof(vio::CommonCfg, queue_select)},
        {"off_queue_size", offsetof(vio::CommonCfg, queue_size)},
        {"off_queue_enable", offsetof(vio::CommonCfg, queue_enable)},
        {"off_queue_notify_off", offsetof(vio::CommonCfg, queue_notify_off)},
        {"off_queue_desc_lo", offsetof(vio::CommonCfg, queue_desc_lo)},
        {"off_queue_driver_lo", offsetof(vio::CommonCfg, queue_driver_lo)},
        {"off_queue_device_lo", offsetof(vio::CommonCfg, queue_device_lo)},
        {"sizeof_desc", sizeof(vio::Desc)},
        {"sizeof_used_elem", sizeof(vio::UsedElem)},
        {"S_ACKNOWLEDGE", vio::S_ACKNOWLEDGE}, {"S_DRIVER", vio::S_DRIVER},
        {"S_DRIVER_OK", vio::S_DRIVER_OK}, {"S_FEATURES_OK", vio::S_FEATURES_OK},
        {"S_NEEDS_RESET", vio::S_NEEDS_RESET}, {"S_FAILED", vio::S_FAILED},
        {"F_VERSION_1", vio::F_VERSION_1},
        {"CAP_COMMON", vio::CAP_COMMON}, {"CAP_NOTIFY", vio::CAP_NOTIFY},
        {"CAP_ISR", vio::CAP_ISR}, {"CAP_DEVICE", vio::CAP_DEVICE},
        {"cap_off_bar", 4}, {"cap_off_offset", 8}, {"notify_cap_off_multiplier", 16},   // virtio.cc
        {"DESC_NEXT", vio::DESC_NEXT}, {"DESC_WRITE", vio::DESC_WRITE},
        {"BLK_T_IN", 0}, {"BLK_T_OUT", 1}, {"BLK_S_OK", 0}, {"sizeof_blk_header", 16},   // virtio_blk.cc
        {"NET_F_MAC", 5}, {"sizeof_net_hdr_v1", 12},                                     // virtio_net.cc
    };
    int failures = 0;
    for (const auto& [k, mine] : ours) {
        const auto it = lx.find(k);
        const bool same = it != lx.end() && it->second == mine;
        if (!same) ++failures;
        std::printf("%-26s ours %4lu  linux %4s  %s\n", k.c_str(), mine,
                    it == lx.end() ? "?" : std::to_string(it->second).c_str(), same ? "same" : "DIFFERENT");
    }
    std::printf("%d difference(s) in %zu values\n", failures, ours.size());
    return failures == 0 ? 0 : 1;
}
