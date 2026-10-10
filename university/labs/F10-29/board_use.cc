// board_use.cc - F10-29 Listing 2: firmware-side code built against the GENERATED header.
// It never names a pin: it asks the board tables. Built by run.sh (after gen_board.py has
// written board_gen.h) with the same host flags as every university listing.
#include <cstdio>
#include <string>
#include <string_view>

#include "board_gen.h"

namespace {

std::string_view dmaStream(std::string_view request)
{
    for (const auto& d : board_gen::kDma) {
        if (d.request == request) {
            return d.stream;
        }
    }
    return {};
}

std::string_view labelOf(std::string_view function)
{
    for (const auto& p : board_gen::kPins) {
        if (p.function == function) {
            return p.label;
        }
    }
    return "-";
}

}  // namespace

int main()
{
    std::printf("board id %d, MCU %.*s\n", board_gen::kBoardId,
                static_cast<int>(board_gen::kMcu.size()), board_gen::kMcu.data());
    int problems = 0;
    for (std::size_t i = 0; i < board_gen::kSerialOrder.size(); ++i) {
        const std::string_view port = board_gen::kSerialOrder[i];
        if (port == "USB") {
            std::printf("SERIAL%zu  USB    (label USB)\n", i);
            continue;
        }
        const std::string rx = std::string(port) + "_RX";
        const std::string tx = std::string(port) + "_TX";
        const std::string_view stream = dmaStream(rx);
        const std::string_view label = labelOf(tx);
        std::printf("SERIAL%zu  %-6.*s (label %.*s): receive by %s%.*s\n", i,
                    static_cast<int>(port.size()), port.data(),
                    static_cast<int>(label.size()), label.data(),
                    stream.empty() ? "INTERRUPT per byte (no DMA)" : "DMA stream ",
                    static_cast<int>(stream.size()), stream.data());
        if (stream.empty() && label.substr(0, 5) == "TELEM") {
            ++problems;
        }
    }
    for (const auto& dev : board_gen::kSpiDevices) {
        const std::string rx = std::string(dev.bus) + "_RX";
        const std::string_view stream = dmaStream(rx);
        std::printf("SPI device %.*s on %.*s, chip select %.*s, receive DMA %.*s\n",
                    static_cast<int>(dev.name.size()), dev.name.data(),
                    static_cast<int>(dev.bus.size()), dev.bus.data(),
                    static_cast<int>(dev.chipSelect.size()), dev.chipSelect.data(),
                    static_cast<int>(stream.size()), stream.data());
    }
    std::printf("board self-check: %s\n",
                problems == 0 ? "PASS" : "WARN (a telemetry port has no receive DMA)");
    return 0;
}
