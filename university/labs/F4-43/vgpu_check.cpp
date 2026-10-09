// vgpu_check.cpp - DR405 F4-43: compare vgpu_proto.h with Linux's <linux/virtio_gpu.h>.
// A difference means our hand-written protocol header is wrong (or Linux's changed).
#include <cstddef>
#include <cstdio>
#include <linux/virtio_gpu.h>
#include <linux/virtio_ids.h>
#include "vgpu_proto.h"

namespace {
int g_bad = 0;
void check(const char* what, unsigned long ours, unsigned long linux_value)
{
    const bool ok = ours == linux_value;
    std::printf("%-44s ours %6lu  linux %6lu  %s\n", what, ours, linux_value, ok ? "ok" : "DIFFERENT");
    if (!ok) ++g_bad;
}
}  // namespace

#define SIZE(ours, theirs) check("sizeof " #theirs, sizeof(vgp::ours), sizeof(struct theirs))
#define OFF(ours, field, theirs, tfield) \
    check("offsetof " #theirs "." #tfield, offsetof(vgp::ours, field), offsetof(struct theirs, tfield))
#define VAL(ours, theirs) check(#theirs, static_cast<unsigned long>(ours), static_cast<unsigned long>(theirs))

int main()
{
    VAL(vgp::PCI_DEVICE_ID - 0x1040, VIRTIO_ID_GPU);
    VAL(vgp::F_EDID, VIRTIO_GPU_F_EDID);
    VAL(vgp::EVENT_DISPLAY, VIRTIO_GPU_EVENT_DISPLAY);
    VAL(vgp::CMD_GET_DISPLAY_INFO, VIRTIO_GPU_CMD_GET_DISPLAY_INFO);
    VAL(vgp::CMD_RESOURCE_CREATE_2D, VIRTIO_GPU_CMD_RESOURCE_CREATE_2D);
    VAL(vgp::CMD_RESOURCE_UNREF, VIRTIO_GPU_CMD_RESOURCE_UNREF);
    VAL(vgp::CMD_SET_SCANOUT, VIRTIO_GPU_CMD_SET_SCANOUT);
    VAL(vgp::CMD_RESOURCE_FLUSH, VIRTIO_GPU_CMD_RESOURCE_FLUSH);
    VAL(vgp::CMD_TRANSFER_TO_HOST_2D, VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D);
    VAL(vgp::CMD_RESOURCE_ATTACH_BACKING, VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING);
    VAL(vgp::CMD_GET_EDID, VIRTIO_GPU_CMD_GET_EDID);
    VAL(vgp::CMD_UPDATE_CURSOR, VIRTIO_GPU_CMD_UPDATE_CURSOR);
    VAL(vgp::CMD_MOVE_CURSOR, VIRTIO_GPU_CMD_MOVE_CURSOR);
    VAL(vgp::RESP_OK_NODATA, VIRTIO_GPU_RESP_OK_NODATA);
    VAL(vgp::RESP_OK_DISPLAY_INFO, VIRTIO_GPU_RESP_OK_DISPLAY_INFO);
    VAL(vgp::RESP_OK_EDID, VIRTIO_GPU_RESP_OK_EDID);
    VAL(vgp::RESP_ERR_UNSPEC, VIRTIO_GPU_RESP_ERR_UNSPEC);
    VAL(vgp::FORMAT_B8G8R8X8_UNORM, VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM);
    VAL(vgp::FORMAT_B8G8R8A8_UNORM, VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM);
    VAL(vgp::MAX_SCANOUTS, VIRTIO_GPU_MAX_SCANOUTS);
    SIZE(CtrlHdr, virtio_gpu_ctrl_hdr);
    OFF(CtrlHdr, fence_id, virtio_gpu_ctrl_hdr, fence_id);
    OFF(CtrlHdr, ring_idx, virtio_gpu_ctrl_hdr, ring_idx);
    SIZE(RespDisplayInfo, virtio_gpu_resp_display_info);
    SIZE(GetEdid, virtio_gpu_cmd_get_edid);
    SIZE(RespEdid, virtio_gpu_resp_edid);
    OFF(RespEdid, edid, virtio_gpu_resp_edid, edid);
    SIZE(ResourceCreate2d, virtio_gpu_resource_create_2d);
    SIZE(MemEntry, virtio_gpu_mem_entry);
    check("sizeof virtio_gpu_resource_attach_backing", sizeof(vgp::AttachBacking) - sizeof(vgp::MemEntry),
          sizeof(struct virtio_gpu_resource_attach_backing));
    SIZE(SetScanout, virtio_gpu_set_scanout);
    OFF(SetScanout, scanout_id, virtio_gpu_set_scanout, scanout_id);
    SIZE(TransferToHost2d, virtio_gpu_transfer_to_host_2d);
    OFF(TransferToHost2d, offset, virtio_gpu_transfer_to_host_2d, offset);
    SIZE(ResourceFlush, virtio_gpu_resource_flush);
    SIZE(UpdateCursor, virtio_gpu_update_cursor);
    OFF(UpdateCursor, resource_id, virtio_gpu_update_cursor, resource_id);
    SIZE(Config, virtio_gpu_config);
    OFF(Config, num_scanouts, virtio_gpu_config, num_scanouts);
    std::printf("%d difference(s)\n", g_bad);
    return g_bad == 0 ? 0 : 1;
}
