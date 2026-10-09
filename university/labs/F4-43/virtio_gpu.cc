// virtio_gpu.cc - DR405 F4-43: virtio-gpu 2D driver. Queue 0 carries control commands,
// queue 1 cursor commands. A control command is a chain of two descriptors: the request
// (the device reads it) and the response buffer (the device writes it).
#include "virtio_gpu.h"
#include "kbase.h"
#include "driver.h"
#include "../F4-05/virtio.h"

namespace {
vio::Device g_dev;
vio::Queue g_ctrl, g_cursor;
alignas(64) uint8_t g_req[512];                        // one request in flight at a time
alignas(64) uint8_t g_resp[sizeof(vgp::RespEdid)];     // the largest response we use
vgp::UpdateCursor g_cur;

volatile vgp::Config& cfg() { return *reinterpret_cast<volatile vgp::Config*>(g_dev.device_cfg); }

vgp::CtrlHdr hdr(uint32_t type) { return vgp::CtrlHdr{type, 0, 0, 0, 0, {0, 0, 0}}; }

// Send 'req' (n bytes), wait for the device, return the response type.
uint32_t command(const void* req, uint32_t n, uint32_t resp_len)
{
    memcpy(g_req, req, n);
    memset(g_resp, 0, resp_len);
    const int head = vio::alloc_chain(g_ctrl, 2);
    if (head < 0) panic("virtio-gpu: no free descriptors");
    vio::Desc* d = g_ctrl.desc;
    const uint16_t h = static_cast<uint16_t>(head), r = d[h].next;
    d[h].addr = reinterpret_cast<uintptr_t>(g_req);
    d[h].len = n;                                      // flags: NEXT (set by alloc_chain)
    d[r].addr = reinterpret_cast<uintptr_t>(g_resp);
    d[r].len = resp_len;
    d[r].flags |= vio::DESC_WRITE;                     // the device writes the response
    vio::submit(g_ctrl, h);
    vio::kick(g_ctrl);
    uint32_t done, len;
    for (uint32_t spins = 0; !vio::get_used(g_ctrl, done, len); ++spins)
        if (spins > 400000000u) panic("virtio-gpu: no response");
    vio::free_chain(g_ctrl, static_cast<uint16_t>(done));
    return reinterpret_cast<const vgp::CtrlHdr*>(g_resp)->type;
}

void cursor_command(const vgp::UpdateCursor& c)
{
    g_cur = c;
    const int head = vio::alloc_chain(g_cursor, 1);
    if (head < 0) panic("virtio-gpu: cursor queue full");
    g_cursor.desc[head].addr = reinterpret_cast<uintptr_t>(&g_cur);
    g_cursor.desc[head].len = sizeof g_cur;
    vio::submit(g_cursor, static_cast<uint16_t>(head));
    vio::kick(g_cursor);
    uint32_t done, len;
    for (uint32_t spins = 0; !vio::get_used(g_cursor, done, len); ++spins)
        if (spins > 400000000u) panic("virtio-gpu: cursor command not consumed");
    vio::free_chain(g_cursor, static_cast<uint16_t>(done));
}
}  // namespace

namespace vgpu {
int init(PciAddr a)
{
    int rc = vio::init(g_dev, a, uint64_t{1} << vgp::F_EDID);
    if (rc) return rc;
    if ((rc = vio::setup_queue(g_dev, g_ctrl, 0))) return rc;
    if ((rc = vio::setup_queue(g_dev, g_cursor, 1))) return rc;
    if (!g_dev.device_cfg) return -E_NODEV;
    vio::driver_ok(g_dev);
    return 0;
}

uint32_t num_scanouts() { return cfg().num_scanouts; }
uint32_t events() { return cfg().events_read; }
void clear_events(uint32_t bits) { cfg().events_clear = bits; }

uint32_t display_info(vgp::RespDisplayInfo& out)
{
    const vgp::CtrlHdr req = hdr(vgp::CMD_GET_DISPLAY_INFO);
    const uint32_t t = command(&req, sizeof req, sizeof out);
    memcpy(&out, g_resp, sizeof out);
    return t;
}

uint32_t edid(uint32_t scanout, vgp::RespEdid& out)
{
    if (!(g_dev.features & (uint64_t{1} << vgp::F_EDID))) return vgp::RESP_ERR_UNSPEC;
    const vgp::GetEdid req{hdr(vgp::CMD_GET_EDID), scanout, 0};
    const uint32_t t = command(&req, sizeof req, sizeof out);
    memcpy(&out, g_resp, sizeof out);
    return t;
}

uint32_t create_2d(uint32_t id, uint32_t format, uint32_t w, uint32_t h)
{
    const vgp::ResourceCreate2d req{hdr(vgp::CMD_RESOURCE_CREATE_2D), id, format, w, h};
    return command(&req, sizeof req, sizeof(vgp::CtrlHdr));
}

uint32_t attach_backing(uint32_t id, const void* mem, uint32_t bytes)
{
    // Paging is off in the lab kernel, so the buffer's address is its physical address and
    // one entry describes it. A kernel with paging lists one entry per physical run.
    const vgp::AttachBacking req{hdr(vgp::CMD_RESOURCE_ATTACH_BACKING), id, 1,
                                 vgp::MemEntry{reinterpret_cast<uintptr_t>(mem), bytes, 0}};
    return command(&req, sizeof req, sizeof(vgp::CtrlHdr));
}

uint32_t set_scanout(uint32_t scanout, uint32_t id, uint32_t w, uint32_t h)
{
    const vgp::SetScanout req{hdr(vgp::CMD_SET_SCANOUT), vgp::Rect{0, 0, w, h}, scanout, id};
    return command(&req, sizeof req, sizeof(vgp::CtrlHdr));
}

uint32_t transfer_2d(uint32_t id, vgp::Rect r, uint32_t stride_bytes)
{
    // offset: where the rectangle's first pixel lies in the guest's backing memory
    const uint64_t offset = uint64_t{r.y} * stride_bytes + uint64_t{r.x} * 4;
    const vgp::TransferToHost2d req{hdr(vgp::CMD_TRANSFER_TO_HOST_2D), r, offset, id, 0};
    return command(&req, sizeof req, sizeof(vgp::CtrlHdr));
}

uint32_t flush(uint32_t id, vgp::Rect r)
{
    const vgp::ResourceFlush req{hdr(vgp::CMD_RESOURCE_FLUSH), r, id, 0};
    return command(&req, sizeof req, sizeof(vgp::CtrlHdr));
}

void update_cursor(uint32_t scanout, uint32_t id, uint32_t x, uint32_t y, uint32_t hot_x, uint32_t hot_y)
{
    cursor_command(vgp::UpdateCursor{hdr(vgp::CMD_UPDATE_CURSOR), vgp::CursorPos{scanout, x, y, 0}, id, hot_x,
                                     hot_y, 0});
}

void move_cursor(uint32_t scanout, uint32_t x, uint32_t y)
{
    cursor_command(vgp::UpdateCursor{hdr(vgp::CMD_MOVE_CURSOR), vgp::CursorPos{scanout, x, y, 0}, 0, 0, 0, 0});
}

const char* resp_name(uint32_t t)
{
    switch (t) {
    case vgp::RESP_OK_NODATA: return "OK_NODATA";
    case vgp::RESP_OK_DISPLAY_INFO: return "OK_DISPLAY_INFO";
    case vgp::RESP_OK_EDID: return "OK_EDID";
    default: return t >= vgp::RESP_ERR_UNSPEC ? "ERROR" : "unexpected";
    }
}
}  // namespace vgpu
