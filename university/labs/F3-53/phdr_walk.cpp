// phdr_walk.cpp - F3-53: what an unwinder can learn about every loaded object through
// dl_iterate_phdr: its program headers, and from PT_GNU_EH_FRAME the start of its sorted
// table of frame descriptions (.eh_frame_hdr). Only the common encoding of the entry count
// (a 4-byte unsigned value, DW_EH_PE_udata4 = 0x03) is decoded; others are reported as such.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <link.h>

namespace {

const char* base_name(const char* path)
{
    if (path == nullptr || path[0] == '\0') return "(the program itself)";
    const char* slash = std::strrchr(path, '/');
    return slash != nullptr ? slash + 1 : path;
}

int visit(dl_phdr_info* info, size_t, void*)
{
    std::printf("%-26s %2d headers", base_name(info->dlpi_name), info->dlpi_phnum);
    bool tls = false;
    const unsigned char* hdr = nullptr;
    for (int i = 0; i < info->dlpi_phnum; ++i) {
        const ElfW(Phdr)& p = info->dlpi_phdr[i];
        if (p.p_type == PT_TLS) tls = true;
        if (p.p_type == PT_GNU_EH_FRAME) {
            hdr = reinterpret_cast<const unsigned char*>(info->dlpi_addr + p.p_vaddr);
        }
    }
    std::printf("  TLS: %-3s", tls ? "yes" : "no");
    if (hdr == nullptr) {
        std::printf("  PT_GNU_EH_FRAME: none\n");
        return 0;
    }
    // .eh_frame_hdr: version, eh_frame_ptr encoding, fde_count encoding, table encoding,
    // then the encoded eh_frame pointer (4 bytes for the 0x1b encoding) and the count
    std::printf("  .eh_frame_hdr: version %u, encodings 0x%02x 0x%02x 0x%02x", hdr[0], hdr[1], hdr[2], hdr[3]);
    if (hdr[1] == 0x1b && hdr[2] == 0x03) {
        std::uint32_t count;
        std::memcpy(&count, hdr + 8, sizeof count);
        std::printf(", %u FDEs in the search table\n", count);
    } else {
        std::printf(", count not decoded\n");
    }
    return 0;
}

} // namespace

int main()
{
    dl_iterate_phdr(visit, nullptr);
    return 0;
}
