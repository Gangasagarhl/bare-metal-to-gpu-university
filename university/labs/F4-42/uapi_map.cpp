// uapi_map.cpp - DR405 F4-42: the kernel side of two GPU paths, as far as the Linux UAPI
// headers installed in the build container show it (linux-libc-dev 6.8.0). Each step is an
// ioctl on a device file; the program decodes each request number into its parts.
#include <cstdio>
#include <drm/amdgpu_drm.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <linux/ioctl.h>
#include <linux/kfd_ioctl.h>

namespace {
void show(const char* file, const char* name, unsigned long req, const char* what)
{
    const unsigned dir = _IOC_DIR(req);
    std::printf("  %-12s %-30s 0x%08lx  type '%c' nr 0x%02lx size %3lu %-5s %s\n", file, name, req,
                static_cast<char>(_IOC_TYPE(req)), static_cast<unsigned long>(_IOC_NR(req)),
                static_cast<unsigned long>(_IOC_SIZE(req)),
                dir == (_IOC_READ | _IOC_WRITE) ? "RW" : dir == _IOC_READ ? "R" : dir == _IOC_WRITE ? "W" : "none",
                what);
}
}  // namespace
#define SHOW(file, req, what) show(file, #req, static_cast<unsigned long>(req), what)

int main()
{
    std::printf("KFD interface version in this header: %d.%d\n\n", KFD_IOCTL_MAJOR_VERSION, KFD_IOCTL_MINOR_VERSION);
    std::printf("Path 1: a compute dispatch through amdkfd (the ROCm runtime, below HIP)\n");
    SHOW("/dev/kfd", AMDKFD_IOC_GET_VERSION, "check the interface version");
    SHOW("/dev/kfd", AMDKFD_IOC_ACQUIRE_VM, "use the GPU address space of a DRM render node");
    SHOW("/dev/kfd", AMDKFD_IOC_ALLOC_MEMORY_OF_GPU, "allocate a buffer (VRAM or system memory)");
    SHOW("/dev/kfd", AMDKFD_IOC_MAP_MEMORY_TO_GPU, "enter it in the GPU page tables");
    SHOW("/dev/kfd", AMDKFD_IOC_CREATE_QUEUE, "create a user-mode queue, get its doorbell");
    SHOW("/dev/kfd", AMDKFD_IOC_CREATE_EVENT, "an event the GPU can signal");
    SHOW("/dev/kfd", AMDKFD_IOC_WAIT_EVENTS, "sleep until the event fires");
    std::printf("  (no ioctl)   write AQL packet + doorbell                    the dispatch itself is a memory write\n");
    std::printf("  create_queue arguments: %zu bytes; queue type COMPUTE_AQL = %d\n\n",
                sizeof(kfd_ioctl_create_queue_args), KFD_IOC_QUEUE_TYPE_COMPUTE_AQL);

    std::printf("Path 2: a command submission through amdgpu (Mesa's OpenGL and Vulkan drivers)\n");
    SHOW("renderD*", DRM_IOCTL_AMDGPU_CTX, "create a submission context");
    SHOW("renderD*", DRM_IOCTL_AMDGPU_GEM_CREATE, "create a buffer object");
    SHOW("renderD*", DRM_IOCTL_AMDGPU_CS, "submit command buffers (IBs) to a ring");
    SHOW("renderD*", DRM_IOCTL_AMDGPU_WAIT_CS, "wait for the submission's fence");
    std::printf("  hardware IP types: GFX=%d COMPUTE=%d DMA=%d\n\n", AMDGPU_HW_IP_GFX, AMDGPU_HW_IP_COMPUTE,
                AMDGPU_HW_IP_DMA);

    std::printf("Path 3: a page flip through KMS (a compositor)\n");
    SHOW("card*", DRM_IOCTL_MODE_GETRESOURCES, "list CRTCs, encoders, connectors");
    SHOW("card*", DRM_IOCTL_MODE_ADDFB2, "wrap a buffer object as a framebuffer");
    SHOW("card*", DRM_IOCTL_MODE_PAGE_FLIP, "show it from the next vertical blank");
    SHOW("card*", DRM_IOCTL_MODE_ATOMIC, "the atomic form of the same change");
    SHOW("card*", DRM_IOCTL_WAIT_VBLANK, "wait for a vertical blank");
    std::printf("  page flip flags: EVENT=0x%x ASYNC=0x%x; struct drm_mode_crtc_page_flip: %zu bytes\n",
                DRM_MODE_PAGE_FLIP_EVENT, DRM_MODE_PAGE_FLIP_ASYNC, sizeof(drm_mode_crtc_page_flip));
    return 0;
}
