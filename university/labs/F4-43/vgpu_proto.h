// vgpu_proto.h - DR405 F4-43: the virtio-gpu 2D protocol, only what this driver uses.
// Written after reading Linux's UAPI header <linux/virtio_gpu.h> as installed in the build
// container (linux-libc-dev 6.8.0-146); the OASIS VIRTIO Specification, "GPU Device", is
// the normative text and was not opened in this build. vgpu_check.cpp compares every
// size, offset and constant below with that header.
#pragma once
#include <stdint.h>

namespace vgp {
constexpr uint16_t PCI_DEVICE_ID = 0x1040 + 16;   // modern virtio-pci: 0x1040 + device id 16
constexpr unsigned F_EDID = 1;                    // feature bit: GET_EDID is supported
constexpr uint32_t EVENT_DISPLAY = 1u << 0;       // config.events_read: display changed

enum : uint32_t {
    CMD_GET_DISPLAY_INFO = 0x0100,
    CMD_RESOURCE_CREATE_2D = 0x0101,
    CMD_RESOURCE_UNREF = 0x0102,
    CMD_SET_SCANOUT = 0x0103,
    CMD_RESOURCE_FLUSH = 0x0104,
    CMD_TRANSFER_TO_HOST_2D = 0x0105,
    CMD_RESOURCE_ATTACH_BACKING = 0x0106,
    CMD_GET_EDID = 0x010A,
    CMD_UPDATE_CURSOR = 0x0300,
    CMD_MOVE_CURSOR = 0x0301,
    RESP_OK_NODATA = 0x1100,
    RESP_OK_DISPLAY_INFO = 0x1101,
    RESP_OK_EDID = 0x1104,
    RESP_ERR_UNSPEC = 0x1200,
};
constexpr uint32_t FORMAT_B8G8R8X8_UNORM = 2;     // bytes in memory: B, G, R, unused
constexpr uint32_t FORMAT_B8G8R8A8_UNORM = 1;     // the same with alpha (cursor images)
constexpr int MAX_SCANOUTS = 16;

struct CtrlHdr { uint32_t type, flags; uint64_t fence_id; uint32_t ctx_id; uint8_t ring_idx, pad[3]; };
struct Rect { uint32_t x, y, width, height; };
struct DisplayOne { Rect r; uint32_t enabled, flags; };
struct RespDisplayInfo { CtrlHdr hdr; DisplayOne pmodes[MAX_SCANOUTS]; };
struct GetEdid { CtrlHdr hdr; uint32_t scanout, padding; };
struct RespEdid { CtrlHdr hdr; uint32_t size, padding; uint8_t edid[1024]; };
struct ResourceCreate2d { CtrlHdr hdr; uint32_t resource_id, format, width, height; };
struct MemEntry { uint64_t addr; uint32_t length, padding; };
struct AttachBacking { CtrlHdr hdr; uint32_t resource_id, nr_entries; MemEntry entry; };  // one entry
struct SetScanout { CtrlHdr hdr; Rect r; uint32_t scanout_id, resource_id; };
struct TransferToHost2d { CtrlHdr hdr; Rect r; uint64_t offset; uint32_t resource_id, padding; };
struct ResourceFlush { CtrlHdr hdr; Rect r; uint32_t resource_id, padding; };
struct CursorPos { uint32_t scanout_id, x, y, padding; };
struct UpdateCursor { CtrlHdr hdr; CursorPos pos; uint32_t resource_id, hot_x, hot_y, padding; };
struct Config { uint32_t events_read, events_clear, num_scanouts, num_capsets; };

static_assert(sizeof(CtrlHdr) == 24, "control header");
static_assert(sizeof(RespDisplayInfo) == 24 + 16 * 24, "display info response");
static_assert(sizeof(RespEdid) == 24 + 8 + 1024, "EDID response");
static_assert(sizeof(AttachBacking) == 32 + 16, "attach backing with one entry");
static_assert(sizeof(TransferToHost2d) == 56 && sizeof(UpdateCursor) == 56, "transfer, cursor");
}  // namespace vgp
