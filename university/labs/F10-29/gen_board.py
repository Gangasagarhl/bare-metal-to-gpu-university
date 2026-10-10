#!/usr/bin/env python3
"""gen_board.py - F10-29 Listing 1: check a board description against the MCU's tables and
generate a C++ header from it. The idea (a text board file -> a generator -> a header the
firmware is built with) is the one ArduPilot uses for hwdef files; this script, its input
format and its checks are the university's own.

usage: gen_board.py <mcu table> <board description> <output header>
exit code: 0 = header written (warnings possible), 1 = errors (no header written)
"""
import sys


def read_lines(path):
    """Yield (line number, list of words) for every non-empty, non-comment line."""
    with open(path, encoding="utf-8") as f:
        for number, raw in enumerate(f, start=1):
            words = raw.split("#", 1)[0].split()
            if words:
                yield number, words


def load_mcu(path):
    pins, dma = {}, {}
    for _, w in read_lines(path):
        if w[0] == "PIN":
            pins[w[1]] = w[2:]
        elif w[0] == "DMA":
            dma[w[1]] = w[2:]
    return pins, dma


NEEDS = {"UART": ("TX", "RX"), "SPI": ("SCK", "MISO", "MOSI"), "I2C": ("SCL", "SDA"),
         "USB": ("DM", "DP")}


def main(mcu_path, board_path, out_path):
    mcu_pins, mcu_dma = load_mcu(mcu_path)
    errors, warnings = [], []
    pins = {}          # pin -> (function, label, extra words)
    labels = {}        # label -> list of pins
    spidevs, imus, serial_order, dma_want = {}, [], [], []
    board_id, mcu_name = None, None
    for n, w in read_lines(board_path):
        key = w[0]
        if key == "MCU":
            mcu_name = w[1]
        elif key == "BOARD_ID":
            board_id = int(w[1])
        elif key == "SPIDEV":
            spidevs[w[1]] = (w[2], w[3])
        elif key == "IMU":
            imus.append((w[1], w[2]))
        elif key == "SERIAL_ORDER":
            serial_order = w[1:]
        elif key == "DMA_WANT":
            dma_want = w[1:]
        elif key in mcu_pins or key[0] == "P":
            pin, func, label = w[0], w[1], w[2]
            if pin not in mcu_pins:
                errors.append(f"line {n}: pin {pin} does not exist on the MCU")
            elif func not in mcu_pins[pin]:
                errors.append(f"line {n}: {pin} cannot be {func} (it can be: "
                              f"{' '.join(mcu_pins[pin])})")
            if pin in pins:
                errors.append(f"line {n}: {pin} already used as {pins[pin][0]}")
            pins[pin] = (func, label, w[3:])
            labels.setdefault(label, []).append(pin)
        else:
            errors.append(f"line {n}: unknown keyword {key}")
    if mcu_name is None or board_id is None:
        errors.append("MCU and BOARD_ID are required")

    # every peripheral that appears must have all of its signals
    used = {}
    for func, _, _ in pins.values():
        if "_" in func and not func.startswith("TIM") and func != "GPIO":
            periph, signal = func.split("_", 1)
            used.setdefault(periph, set()).add(signal)
    for periph, signals in sorted(used.items()):
        kind = periph.rstrip("0123456789")
        missing = [s for s in NEEDS.get(kind, ()) if s not in signals]
        if missing:
            errors.append(f"{periph}: missing signal(s) {' '.join(missing)}")

    for name, (bus, cs) in spidevs.items():
        if bus not in used:
            errors.append(f"SPIDEV {name}: bus {bus} has no pins")
        cs_pins = labels.get(cs, [])
        if len(cs_pins) != 1 or "OUTPUT" not in pins[cs_pins[0]][2]:
            errors.append(f"SPIDEV {name}: chip select {cs} must be one GPIO OUTPUT pin")
    for dev, _ in imus:
        if dev not in spidevs:
            errors.append(f"IMU {dev}: no SPIDEV of that name")
    for port in serial_order:
        if port not in used:
            errors.append(f"SERIAL_ORDER: {port} has no pins")

    # DMA: give each wanted request the first free stream, in DMA_WANT order (greedy)
    owner, dma = {}, {}
    for req in dma_want:
        periph = req.split("_", 1)[0]
        if periph not in used and not periph.startswith("TIM"):
            errors.append(f"DMA_WANT {req}: peripheral not on any pin")
            continue
        options = mcu_dma.get(req, [])
        free = [s for s in options if s not in owner]
        if free:
            owner[free[0]] = req
            dma[req] = free[0]
        else:
            taken = ", ".join(f"{s} by {owner[s]}" for s in options) or "none exist"
            warnings.append(f"no free DMA stream for {req} (its streams: {taken}); "
                            f"{req} will run WITHOUT DMA (one interrupt per byte or word)")
            dma[req] = None

    print(f"board {board_path}: MCU {mcu_name}, board id {board_id}")
    print(f"  pins used: {len(pins)}, SPI devices: {len(spidevs)}, IMUs: {len(imus)}")
    for req in dma_want:
        print(f"  DMA {req:9s} -> {dma.get(req) or 'none'}")
    for i, port in enumerate(serial_order):
        print(f"  SERIAL{i} = {port}")
    for msg in warnings:
        print("WARNING:", msg)
    for msg in errors:
        print("ERROR:", msg)
    if errors:
        print(f"{len(errors)} error(s): no header written")
        return 1

    out = ["// board_gen.h - GENERATED by gen_board.py from " + board_path + ". Do not edit.",
           "#pragma once", "#include <array>", "#include <string_view>", "",
           "namespace board_gen {", "",
           f"inline constexpr int kBoardId = {board_id};",
           f'inline constexpr std::string_view kMcu = "{mcu_name}";', "",
           "struct Pin { std::string_view pin, function, label; };",
           f"inline constexpr std::array<Pin, {len(pins)}> kPins{{{{"]
    out += [f'    {{"{p}", "{f}", "{lab}"}},' for p, (f, lab, _) in pins.items()]
    out += ["}};", "",
            f"inline constexpr std::array<std::string_view, {len(serial_order)}> kSerialOrder{{{{"]
    out += [f'    "{p}",' for p in serial_order]
    out += ["}};", "", "struct Dma { std::string_view request, stream; };  // stream \"\" = none",
            f"inline constexpr std::array<Dma, {len(dma_want)}> kDma{{{{"]
    out += [f'    {{"{r}", "{dma.get(r) or ""}"}},' for r in dma_want]
    out += ["}};", "", "struct SpiDevice { std::string_view name, bus, chipSelect; };",
            f"inline constexpr std::array<SpiDevice, {len(spidevs)}> kSpiDevices{{{{"]
    out += [f'    {{"{nm}", "{b}", "{c}"}},' for nm, (b, c) in spidevs.items()]
    out += ["}};", "", "}  // namespace board_gen", ""]
    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print(f"{len(warnings)} warning(s); header written: {out_path}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    sys.exit(main(sys.argv[1], sys.argv[2], sys.argv[3]))
