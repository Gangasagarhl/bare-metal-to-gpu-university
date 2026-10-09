/* linux_values.c - DR301 F4-05: print the values Linux's UAPI headers give for the virtio
 * structures and constants used by virtio.h. Some of these headers are valid only as C,
 * so this half of the check is a C program; virtio_check.cc compares. */
#include <stddef.h>
#include <stdio.h>
#include <linux/virtio_blk.h>
#include <linux/virtio_config.h>
#include <linux/virtio_net.h>
#include <linux/virtio_pci.h>
#include <linux/virtio_ring.h>

#define P(name, value) printf("%s %lu\n", name, (unsigned long)(value))

int main(void)
{
    P("sizeof_common_cfg", sizeof(struct virtio_pci_common_cfg));
    P("off_device_feature_select", offsetof(struct virtio_pci_common_cfg, device_feature_select));
    P("off_device_feature", offsetof(struct virtio_pci_common_cfg, device_feature));
    P("off_driver_feature_select", offsetof(struct virtio_pci_common_cfg, guest_feature_select));
    P("off_driver_feature", offsetof(struct virtio_pci_common_cfg, guest_feature));
    P("off_num_queues", offsetof(struct virtio_pci_common_cfg, num_queues));
    P("off_device_status", offsetof(struct virtio_pci_common_cfg, device_status));
    P("off_queue_select", offsetof(struct virtio_pci_common_cfg, queue_select));
    P("off_queue_size", offsetof(struct virtio_pci_common_cfg, queue_size));
    P("off_queue_enable", offsetof(struct virtio_pci_common_cfg, queue_enable));
    P("off_queue_notify_off", offsetof(struct virtio_pci_common_cfg, queue_notify_off));
    P("off_queue_desc_lo", offsetof(struct virtio_pci_common_cfg, queue_desc_lo));
    P("off_queue_driver_lo", offsetof(struct virtio_pci_common_cfg, queue_avail_lo));
    P("off_queue_device_lo", offsetof(struct virtio_pci_common_cfg, queue_used_lo));
    P("sizeof_desc", sizeof(struct vring_desc));
    P("sizeof_used_elem", sizeof(struct vring_used_elem));
    P("S_ACKNOWLEDGE", VIRTIO_CONFIG_S_ACKNOWLEDGE);
    P("S_DRIVER", VIRTIO_CONFIG_S_DRIVER);
    P("S_DRIVER_OK", VIRTIO_CONFIG_S_DRIVER_OK);
    P("S_FEATURES_OK", VIRTIO_CONFIG_S_FEATURES_OK);
    P("S_NEEDS_RESET", VIRTIO_CONFIG_S_NEEDS_RESET);
    P("S_FAILED", VIRTIO_CONFIG_S_FAILED);
    P("F_VERSION_1", VIRTIO_F_VERSION_1);
    P("CAP_COMMON", VIRTIO_PCI_CAP_COMMON_CFG);
    P("CAP_NOTIFY", VIRTIO_PCI_CAP_NOTIFY_CFG);
    P("CAP_ISR", VIRTIO_PCI_CAP_ISR_CFG);
    P("CAP_DEVICE", VIRTIO_PCI_CAP_DEVICE_CFG);
    P("cap_off_bar", offsetof(struct virtio_pci_cap, bar));
    P("cap_off_offset", offsetof(struct virtio_pci_cap, offset));
    P("notify_cap_off_multiplier", offsetof(struct virtio_pci_notify_cap, notify_off_multiplier));
    P("DESC_NEXT", VRING_DESC_F_NEXT);
    P("DESC_WRITE", VRING_DESC_F_WRITE);
    P("BLK_T_IN", VIRTIO_BLK_T_IN);
    P("BLK_T_OUT", VIRTIO_BLK_T_OUT);
    P("BLK_S_OK", VIRTIO_BLK_S_OK);
    P("sizeof_blk_header", sizeof(struct virtio_blk_outhdr));
    P("NET_F_MAC", VIRTIO_NET_F_MAC);
    P("sizeof_net_hdr_v1", sizeof(struct virtio_net_hdr_v1));
    return 0;
}
