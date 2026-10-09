// pm.cc - DR302 F4-11: FADT fixed hardware (see pm.h).
#include "pm.h"
#include "kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-08/intr.h"

namespace {
template <typename T> T at(const uint8_t* t, uint32_t off)
{
    T v;
    memcpy(&v, t + off, sizeof v);
    return v;
}
}

namespace pm {
bool read_fadt(Fadt& f)
{
    const AcpiHeader* h = acpi::find("FACP");
    if (!h) return false;
    const auto* t = reinterpret_cast<const uint8_t*>(h);
    f.dsdt = at<uint32_t>(t, fadt::DSDT);
    if (h->length >= fadt::X_DSDT + 8 && at<uint64_t>(t, fadt::X_DSDT) != 0 && at<uint64_t>(t, fadt::X_DSDT) < (1ull << 32))
        f.dsdt = static_cast<uint32_t>(at<uint64_t>(t, fadt::X_DSDT));   // ACPI 2.0+: the 64-bit field wins
    f.sci_irq = at<uint16_t>(t, fadt::SCI_INT);
    f.smi_cmd = at<uint32_t>(t, fadt::SMI_CMD);
    f.acpi_enable = t[fadt::ACPI_ENABLE];
    f.acpi_disable = t[fadt::ACPI_DISABLE];
    f.pm1a_evt = at<uint32_t>(t, fadt::PM1A_EVT_BLK);
    f.pm1b_evt = at<uint32_t>(t, fadt::PM1B_EVT_BLK);
    f.pm1a_cnt = at<uint32_t>(t, fadt::PM1A_CNT_BLK);
    f.pm1b_cnt = at<uint32_t>(t, fadt::PM1B_CNT_BLK);
    f.pm_tmr = at<uint32_t>(t, fadt::PM_TMR_BLK);
    f.gpe0 = at<uint32_t>(t, fadt::GPE0_BLK);
    f.pm1_evt_len = t[fadt::PM1_EVT_LEN];
    f.pm1_cnt_len = t[fadt::PM1_CNT_LEN];
    f.pm_tmr_len = t[fadt::PM_TMR_LEN];
    f.gpe0_len = t[fadt::GPE0_BLK_LEN];
    f.flags = at<uint32_t>(t, fadt::FLAGS);
    f.tmr32 = (f.flags & fadt::FLAG_TMR_VAL_EXT) != 0;
    return true;
}

void print(const Fadt& f)
{
    kprintf("fadt: DSDT at 0x%x, SCI interrupt %u, SMI_CMD port 0x%x (ACPI_ENABLE 0x%x, ACPI_DISABLE 0x%x)\n", f.dsdt,
            f.sci_irq, f.smi_cmd, f.acpi_enable, f.acpi_disable);
    kprintf("fadt: PM1a_EVT 0x%x (%u bytes: status, then enable)  PM1b_EVT 0x%x  PM1a_CNT 0x%x (%u bytes)  PM1b_CNT 0x%x\n",
            f.pm1a_evt, f.pm1_evt_len, f.pm1b_evt, f.pm1a_cnt, f.pm1_cnt_len, f.pm1b_cnt);
    kprintf("fadt: PM_TMR 0x%x (%u bits)  GPE0 0x%x (%u bytes)  flags 0x%x\n", f.pm_tmr, f.tmr32 ? 32 : 24, f.gpe0,
            f.gpe0_len, f.flags);
}

uint16_t control(const Fadt& f) { return inw(static_cast<uint16_t>(f.pm1a_cnt)); }

bool enable_acpi_mode(const Fadt& f)
{
    if (control(f) & pm1::SCI_EN) {
        kprintf("acpi: SCI_EN already set: the firmware left the machine in ACPI mode\n");
        return true;
    }
    if (f.smi_cmd == 0 || f.acpi_enable == 0) {
        kprintf("acpi: SCI_EN clear and no SMI_CMD: hardware-reduced or broken firmware\n");
        return false;
    }
    outb(static_cast<uint16_t>(f.smi_cmd), f.acpi_enable);   // ask the firmware (SMM) to hand over
    const uint64_t end = clock::ms() + 3000;
    while (clock::ms() < end)
        if (control(f) & pm1::SCI_EN) { kprintf("acpi: SCI_EN set after writing 0x%x to port 0x%x\n", f.acpi_enable, f.smi_cmd); return true; }
    kprintf("acpi: SCI_EN still clear 3 s after the ACPI_ENABLE request\n");
    return false;
}

uint32_t timer(const Fadt& f)
{
    const uint32_t v = inl(static_cast<uint16_t>(f.pm_tmr));
    return f.tmr32 ? v : (v & 0xFFFFFF);
}

uint16_t status(const Fadt& f)
{
    uint16_t s = inw(static_cast<uint16_t>(f.pm1a_evt));
    if (f.pm1b_evt) s |= inw(static_cast<uint16_t>(f.pm1b_evt));
    return s;
}

void clear_status(const Fadt& f, uint16_t bits)
{
    outw(static_cast<uint16_t>(f.pm1a_evt), bits);            // writing 1 clears; 0 leaves alone
    if (f.pm1b_evt) outw(static_cast<uint16_t>(f.pm1b_evt), bits);
}

void set_enable(const Fadt& f, uint16_t bits)
{
    const uint16_t en_a = static_cast<uint16_t>(f.pm1a_evt + f.pm1_evt_len / 2);   // enable half
    outw(en_a, static_cast<uint16_t>(inw(en_a) | bits));
    if (f.pm1b_evt) {
        const uint16_t en_b = static_cast<uint16_t>(f.pm1b_evt + f.pm1_evt_len / 2);
        outw(en_b, static_cast<uint16_t>(inw(en_b) | bits));
    }
}

void sleep(const Fadt& f, uint8_t slp_typa, uint8_t slp_typb)
{
    clear_status(f, pm1::WAK_STS);
    const uint16_t a = static_cast<uint16_t>((control(f) & ~pm1::SLP_TYP_MASK) | (slp_typa << pm1::SLP_TYP_SHIFT) | pm1::SLP_EN);
    kprintf("acpi: S5: writing 0x%04x to PM1a_CNT (port 0x%x)\n", a, f.pm1a_cnt);
    intr::disable();
#ifdef F411_BYTE_WRITE
    // The bug of the forensic build: a byte write. PM1_CNT is 2 bytes wide and SLP_EN is bit 13.
    outb(static_cast<uint16_t>(f.pm1a_cnt), static_cast<uint8_t>(a));
#else
    outw(static_cast<uint16_t>(f.pm1a_cnt), a);
#endif
    if (f.pm1b_cnt) {
        const uint16_t b = static_cast<uint16_t>((inw(static_cast<uint16_t>(f.pm1b_cnt)) & ~pm1::SLP_TYP_MASK) |
                                                 (slp_typb << pm1::SLP_TYP_SHIFT) | pm1::SLP_EN);
        outw(static_cast<uint16_t>(f.pm1b_cnt), b);
    }
    intr::enable();
}
}  // namespace pm
