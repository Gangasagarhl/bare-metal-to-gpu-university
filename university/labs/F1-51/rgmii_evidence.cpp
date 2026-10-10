// rgmii_evidence.cpp - F1-51 forensic evidence generator: "link up, no packets".
// Prints what the engineer saw on board revision B, in the simulator's own log format,
// including a dump of the toy PHY's registers (a made-up register map, see the chapter).
#include "rgmii_model.h"

#include <cstdio>

int main()
{
    // Board revision B: the PHY's receive-clock delay is switched on by its pin strapping,
    // and the new MAC driver configuration ALSO asks the MAC to delay the receive clock.
    const bool phyRxDelay = true;
    const bool macRxDelay = true;
    const int boardDelay = 0;          // the board trace adds no delay on this revision
    const int unit = 2;                // ticks added by one "delay on" switch
    const int total = (phyRxDelay ? unit : 0) + (macRxDelay ? unit : 0) + boardDelay;

    std::printf("[  1.20] mac0: probe ok, MAC address 52:54:00:12:34:56\n");
    std::printf("[  1.21] mac0: config: interface=clock-forwarded DDR, mac_rx_clock_delay=%s, mac_tx_clock_delay=off\n",
                macRxDelay ? "on" : "off");
    std::printf("[  3.05] mac0: PHY reports link up, gigabit, full duplex\n");
    const int sent = 20;
    const int good = toy::receiveGood(total, sent, 7);
    std::printf("[ 13.05] mac0: tx_frames=%d tx_errors=0\n", sent);
    std::printf("[ 13.05] mac0: rx_frames_ok=%d rx_crc_errors=%d rx_dropped=0\n", good, sent - good);
    std::printf("[ 13.06] net: no reply to %d ARP requests for the gateway\n", sent);
    std::printf("\ntoy PHY register dump (read over the management bus, MDIO):\n");
    std::printf("  reg 0x01 status       = 0x%04X   (bit 2 LINK = 1, bit 5 AUTONEG_DONE = 1)\n",
                (1u << 2) | (1u << 5));
    std::printf("  reg 0x02 id           = 0x%04X   (toy vendor id)\n", 0x7E57u);
    std::printf("  reg 0x14 delay_ctrl   = 0x%04X   (bit 0 RX_CLK_DELAY = %d, bit 1 TX_CLK_DELAY = %d)\n",
                (phyRxDelay ? 1u : 0u) | (1u << 1), phyRxDelay ? 1 : 0, 1);
    std::printf("  reg 0x15 strap_status = 0x%04X   (bit 0 RX_DELAY_STRAP = 1)\n", 1u);
    std::printf("\nboard B schematic note: RX clock trace length matched to RX data (no extra delay)\n");
    std::printf("board A (works) used the previous MAC configuration: mac_rx_clock_delay=off\n");
    return 0;
}
