// regs_layer.cpp - DR301 F4-01: the register-access layer of reg.h, tested on the host.
// A FakeBus stands in for the hardware: it stores register values and logs every access,
// so the test can check the exact sequence of reads and writes a driver performs.
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "reg.h"
#include "uart_regs.h"

struct FakeBus {
    std::map<uint32_t, uint32_t>* regs;
    std::vector<std::string>* log;
    template <typename T> T read(uint32_t off) const
    {
        const T v = static_cast<T>((*regs)[off]);
        log->push_back("read  +" + std::to_string(off) + " -> " + std::to_string(v));
        return v;
    }
    template <typename T> void write(uint32_t off, T v) const
    {
        log->push_back("write +" + std::to_string(off) + " <- " + std::to_string(v));
        (*regs)[off] = v;
    }
};

// A made-up device with a 32-bit status register whose bits are write-1-to-clear.
namespace dev {
using CTRL = Reg<uint32_t, 0x00, Access::RW>;
using STATUS = Reg<uint32_t, 0x04, Access::RW1C>;
using ID = Reg<uint32_t, 0x08, Access::RO>;
using CTRL_SPEED = Field<CTRL, 4, 3>;      // bits 4..6
}

int main()
{
    std::map<uint32_t, uint32_t> regs{{0x00, 0x0000'0101}, {0x04, 0x0000'0005}, {0x08, 0xC0DE'0001}};
    std::vector<std::string> log;
    RegBlock<FakeBus> d{FakeBus{&regs, &log}};

    std::printf("ID = 0x%08x\n", d.read<dev::ID>());
    d.set_field<dev::CTRL_SPEED>(5);       // read-modify-write keeps bits 0 and 8
    std::printf("CTRL after set_field(SPEED=5) = 0x%08x (expected 0x00000151)\n", regs[0x00]);
    std::printf("SPEED field reads back %u\n", d.get_field<dev::CTRL_SPEED>());
    d.clear<dev::STATUS>(0x4);             // clear bit 2 only: write a 1 to it
    std::printf("bytes written to STATUS: 0x%x (a real RW1C register would now hold 0x1)\n", regs[0x04]);
    std::printf("Field mask of CTRL_SPEED = 0x%08x\n", dev::CTRL_SPEED::mask);

    std::map<uint32_t, uint32_t> uregs{{5, 0x60}};
    RegBlock<FakeBus> u{FakeBus{&uregs, &log}};
    log.clear();
    u.write<uart::LCR>(uart::LCR_DLAB);    // the 16550 divisor sequence of uart16550.cc
    u.write<uart::DLL>(0x01);
    u.write<uart::DLM>(0x00);
    u.write<uart::LCR>(uart::LCR_8N1);
    std::printf("16550 divisor sequence, as the device sees it:\n");
    for (const auto& line : log) std::printf("  %s\n", line.c_str());
    const bool ok = regs[0x00] == 0x151 && d.get_field<dev::CTRL_SPEED>() == 5 && uregs[3] == 0x03;
    std::printf("%s\n", ok ? "all checks passed" : "CHECK FAILED");
    return ok ? 0 : 1;
}
