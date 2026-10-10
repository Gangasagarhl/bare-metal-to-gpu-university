// pm.h - DR302 F4-11: the fixed ACPI hardware the FADT describes. PM1 event and control
// blocks, the PM timer, the switch into ACPI mode, the System Control Interrupt (SCI), and
// the sleep-state write that powers the machine off.
// FADT offsets and PM1 bit positions: ACPI specification, "Fixed ACPI Description Table" and
// "PM1 Event / Control Registers" (tier 1, written from memory in this build: the run checks
// them only against what QEMU does, see the chapter's unverified box).
#pragma once
#include <stdint.h>

namespace fadt {                         // byte offsets in the FADT
constexpr uint32_t DSDT = 40, SCI_INT = 46, SMI_CMD = 48, ACPI_ENABLE = 52, ACPI_DISABLE = 53,
                   PM1A_EVT_BLK = 56, PM1B_EVT_BLK = 60, PM1A_CNT_BLK = 64, PM1B_CNT_BLK = 68,
                   PM_TMR_BLK = 76, GPE0_BLK = 80, PM1_EVT_LEN = 88, PM1_CNT_LEN = 89, PM_TMR_LEN = 91,
                   GPE0_BLK_LEN = 92, FLAGS = 112, RESET_REG = 116, RESET_VALUE = 128, X_DSDT = 140;
constexpr uint32_t FLAG_TMR_VAL_EXT = 1u << 8;     // PM timer is 32 bits wide (else 24)
}

namespace pm1 {                          // bits of the PM1 status / enable / control registers
constexpr uint16_t TMR_STS = 1u << 0, PWRBTN_STS = 1u << 8, WAK_STS = 1u << 15;
constexpr uint16_t TMR_EN = 1u << 0, PWRBTN_EN = 1u << 8;
constexpr uint16_t SCI_EN = 1u << 0, SLP_TYP_SHIFT = 10, SLP_TYP_MASK = 7u << 10, SLP_EN = 1u << 13;
constexpr uint32_t PM_TIMER_HZ = 3579545;          // the PM timer's fixed frequency
}

namespace pm {
struct Fadt {
    uint32_t dsdt;
    uint16_t sci_irq;
    uint32_t smi_cmd;
    uint8_t acpi_enable, acpi_disable;
    uint32_t pm1a_evt, pm1b_evt, pm1a_cnt, pm1b_cnt, pm_tmr, gpe0;
    uint8_t pm1_evt_len, pm1_cnt_len, pm_tmr_len, gpe0_len;
    uint32_t flags;
    bool tmr32;
};
bool read_fadt(Fadt& f);                  // false: no FADT
void print(const Fadt& f);
bool enable_acpi_mode(const Fadt& f);     // SCI_EN set (by us through SMI_CMD, or already)
uint32_t timer(const Fadt& f);            // the free-running PM timer
uint16_t status(const Fadt& f);           // PM1a_STS (| PM1b_STS)
void clear_status(const Fadt& f, uint16_t bits);   // write-1-to-clear
void set_enable(const Fadt& f, uint16_t bits);
uint16_t control(const Fadt& f);
// Enters sleep state 'slp_typ' (from \_Sx): SLP_TYP and SLP_EN in PM1a_CNT (and PM1b_CNT).
void sleep(const Fadt& f, uint8_t slp_typa, uint8_t slp_typb);
}
