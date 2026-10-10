// hda.h - DR302 F4-12: an Intel High Definition Audio controller driver and codec walker.
// Register offsets: Intel "High Definition Audio Specification" rev 1.0a, section 3.3
// (controller registers) and section 7.3 (codec verbs and parameters). Tier 1, written from
// memory in this build and checked only against QEMU's intel-hda + hda-output models.
#pragma once
#include <stdint.h>
#include "../F4-02/pci.h"

namespace hdareg {                       // controller registers (offsets in BAR0)
constexpr uint32_t GCAP = 0x00, VMIN = 0x02, VMAJ = 0x03, GCTL = 0x08, STATESTS = 0x0E, INTCTL = 0x20,
                   INTSTS = 0x24, WALCLK = 0x30, CORBLBASE = 0x40, CORBUBASE = 0x44, CORBWP = 0x48,
                   CORBRP = 0x4A, CORBCTL = 0x4C, CORBSTS = 0x4D, CORBSIZE = 0x4E, RIRBLBASE = 0x50,
                   RIRBUBASE = 0x54, RIRBWP = 0x58, RINTCNT = 0x5A, RIRBCTL = 0x5C, RIRBSTS = 0x5D,
                   RIRBSIZE = 0x5E;
constexpr uint32_t GCTL_CRST = 1u << 0;
constexpr uint32_t INTCTL_GIE = 1u << 31, INTCTL_CIE = 1u << 30;
constexpr uint16_t CORBRP_RST = 1u << 15, RIRBWP_RST = 1u << 15;
constexpr uint8_t CORBCTL_RUN = 1u << 1, RIRBCTL_RUN = 1u << 1, RIRBCTL_INT = 1u << 0;
// Stream descriptors: input streams first, then output, then bidirectional; 0x20 apart.
constexpr uint32_t SD_BASE = 0x80, SD_SIZE = 0x20;
constexpr uint32_t SD_CTL = 0x00, SD_STS = 0x03, SD_LPIB = 0x04, SD_CBL = 0x08, SD_LVI = 0x0C,
                   SD_FIFOS = 0x10, SD_FMT = 0x12, SD_BDPL = 0x18, SD_BDPU = 0x1C;
constexpr uint32_t SDCTL_SRST = 1u << 0, SDCTL_RUN = 1u << 1, SDCTL_IOCE = 1u << 2, SDCTL_STRM_SHIFT = 20;
constexpr uint8_t SDSTS_BCIS = 1u << 2, SDSTS_FIFOE = 1u << 3, SDSTS_DESE = 1u << 4;
}

namespace verb {                         // codec verbs: 12-bit verb + 8-bit payload, or 4-bit + 16-bit
constexpr uint32_t GET_PARAM = 0xF00, GET_CONN_LIST = 0xF02, SET_POWER = 0x705, SET_STREAM_ID = 0x706,
                   SET_PIN_CTL = 0x707, SET_EAPD = 0x70C, GET_CONFIG_DEFAULT = 0xF1C;
constexpr uint32_t SET_FORMAT4 = 0x2, SET_AMP4 = 0x3, GET_FORMAT4 = 0xA, GET_AMP4 = 0xB;
// GET_PARAM parameter ids
constexpr uint8_t P_VENDOR = 0x00, P_REVISION = 0x02, P_NODES = 0x04, P_FG_TYPE = 0x05, P_WCAP = 0x09,
                  P_PCM = 0x0A, P_FORMATS = 0x0B, P_PIN_CAP = 0x0C, P_CONN_LEN = 0x0E, P_OUT_AMP = 0x12;
constexpr uint32_t PIN_OUT_EN = 0x40;
// SET_AMP payload: bit 15 output amp, 14 input amp, 13 left, 12 right, 7 mute, 6-0 gain step.
// GET_AMP payload: bit 15 output (else input), bit 13 left (else right).
constexpr uint32_t AMP_OUT = 1u << 15, AMP_LEFT = 1u << 13, AMP_RIGHT = 1u << 12, AMP_MUTE = 1u << 7;
}

namespace hda {
// The 16-bit stream format of the controller (SDnFMT) and the converter (SET_FORMAT):
// bit 14 base rate (0 = 48 kHz, 1 = 44.1 kHz), bits 13-11 multiply, 10-8 divide,
// bits 6-4 sample size (1 = 16 bits), bits 3-0 number of channels MINUS ONE.
uint16_t format(uint32_t rate, uint8_t bits, uint8_t channels);
void describe_format(uint16_t fmt, char* out, uint32_t cap);

bool find(PciAddr& a);
// Reset, CORB/RIRB set up, codecs found. 'vector' 0 = poll the stream status instead.
bool init(PciAddr a, uint8_t vector);
uint16_t codecs();                       // STATESTS after reset: one bit per codec address
uint32_t command(uint8_t cad, uint8_t nid, uint32_t verb20);   // through CORB, answer from RIRB
uint32_t param(uint8_t cad, uint8_t nid, uint8_t id);

struct Path { uint8_t cad, afg, dac, pin; };
// Prints every widget of codec 'cad' and picks the first output converter wired to an
// output-capable pin. False if there is none.
bool walk(uint8_t cad, Path& out);

// Plays 'bytes' of PCM from 'buf' (a ring of 'periods' equal parts, one interrupt each)
// until 'stop_after' periods have completed. Returns the number completed.
struct PlayStats { uint32_t periods, irqs, lpib_last, fifoe; uint64_t ms; };
PlayStats play(const Path& p, const void* buf, uint32_t bytes, uint16_t fmt, uint32_t periods, uint32_t stop_after);
void irq(uint8_t vector);                // the MSI handler
}
