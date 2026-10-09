# DS201 — Networking fundamentals: author notes

These notes cover chapters F5-01 to F5-07, all at level L2. The prerequisites are SP102 and KID101 (F0-09).

This course maps to curriculum item 50 and previews milestone C9. Course labs:

- an annotated Wireshark capture (F5-04)
- a TCP echo server and client (F5-05)
- latency and throughput measurement (F5-07)

Course forensics:

- "The slow download" (F5-04)
- "Forty milliseconds of nothing" (F5-07)
- one forensic per chapter in the other chapters

Labs are in `university/labs/F5-01` to `university/labs/F5-07`. Each one passes `university/labs/run_lab.sh university/labs/<ID>` with status 0. All seven were run again, in order from F5-01 to F5-07, at the end of this build on 2026-10-09. F5-04 was run once more after its `run.sh` changed to list every retransmission. F5-06 was run again after the zone name changed to `web.lab.example`. The numbers quoted in the prose come from these final runs.

## Toolchain (as recorded in the logs)

- **Compiler:** g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0. The course flags are `-std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined`. F5-07 also has `-O2` builds without sanitizers, which are labelled as such.
- **Capture tool:** TShark (Wireshark) 4.2.2.
- **Machine:** a cloud build container shared with other jobs.
  - Linux 6.18.44-fc-v80, x86_64.
  - 4 logical CPUs, reported as "Intel(R) Xeon(R) Processor @ 2.10GHz".
  - IPv6 disabled.
  - No `ip` or `tc` tools.
  - Only the loopback interface is used.

## How the network runs were isolated (owner decision 1)

Programs that need the network are kept in `net/` subfolders, so `run_lab.sh` does not run them directly. Each lab's `run.sh` runs them instead, inside a private network namespace.

- **F5-01 to F5-05 and F5-07** use `unshare -n`. Loopback is brought up there with a Python `SIOCSIFFLAGS` ioctl.
- **F5-06** uses `unshare -n -m --propagation private`. Inside that namespace a bind mount replaces `/etc/resolv.conf` with `nameserver 127.0.0.1`. The machine's real `/etc/resolv.conf` was checked before and after the runs and is unchanged.
- **F5-04** adds its `iptables` packet-drop rule (every 40th data packet from port 41000) only inside its own namespace.

All of this needs root. If `unshare` is not allowed, each `run.sh` writes logs that say "untested in this environment" instead of failing.

**Decision for the owner:** should learners on their own machines skip `run.sh` and run on plain loopback? The chapters already say that this works without root, except the F5-06 resolver redirect and the F5-04 packet loss.

## Listings run

Every listing was built and run. **Nothing ran on a physical network.**

| Chapter | Run (`.log`) | Exit / result | Status |
|---|---|---|---|
| F5-01 | header_bytes, channel, receiver_log, receiver_fixed | 0 | pass (receiver_log is forensic evidence: the memcpy byte-order bug) |
| F5-01 | send_one, send_one_capture, lo_mtu (netns) | 0 | pass |
| F5-02 | encapsulate, handmade_decode, demux, port53_decode, zero_checksum_decode | 0 | pass |
| F5-02 | broken_frame, broken_decode | 0 | forensic (checksum computed before the payload) |
| F5-02 | kernel_decode (decodes `../F5-01/send_one.pcapng`) | 0 | pass |
| F5-03 | ipcalc, route, arp_request, arp_decode | 0 | pass |
| F5-03 | printer_case, printer_capture | 0 | forensic, simulated frames |
| F5-03 | show_addrs (netns) | 0 | pass |
| F5-04 | boundaries | 0 | pass |
| F5-04 | tcp_talk, tcp_capture, tcp_fields, refused, refused_capture (netns) | 0 | pass |
| F5-04 | download_runs, slow_capture (netns, iptables loss) | 0 | forensic evidence |
| F5-05 | partial_reads, echo_server, echo_client (13 B, 1 MiB, 100 MiB, all PASS) | 0 | pass |
| F5-05 | truncated | 0 | forensic (single-`recv` client) |
| F5-05 | deadlock (`deadlock.timeout` = 3) | 124 | expected-fail: the time limit ends the deadlock on purpose |
| F5-06 | dns_server, dns_capture (netns) | 0 | pass |
| F5-06 | resolve (netns) | 1 | expected: `printer.lab.example` is NXDOMAIN, which is the forensic |
| F5-06 | dhcp_frames, dhcp_decode | 0 | pass, simulated frames |
| F5-07 | machine, udp_rtt, tcp_throughput, small_requests, the `_O2` runs, small_requests_capture (netns) | 0 | pass |

### Untested on hardware

Each of these chapters has a box that says what is untested:

- **F5-03:** real ARP on an Ethernet or Wi-Fi network, and lab step 6 (the learner's own routing table). `ip route` was not available in the build.
- **F5-06:** a real DHCP exchange (lab step 6). The DHCP frames were built by `dhcp_frames.cpp` and checked only by tshark's decoder. AAAA lab step 3 was not run because IPv6 is disabled in the container.
- **F5-07:** the real-link half of the course lab (two machines on a wire). All numbers are from loopback on a shared VM.

## Unverified boxes (AH-19)

- **F5-01:** the typical Ethernet MTU, the minimum IPv4 reassembly size, and real-world rates of loss, duplication and reordering.
- **F5-02:**
  - how Linux marks loopback packets so that checksum verification is skipped;
  - the classic pcap file layout used by `writePcap` (written from memory; the only check is that tshark accepts it, see owner decision 3).
- **F5-03:** the RFC 1918 private ranges, the RFC 5737 documentation ranges, and other address facts listed in the box.
- **F5-04:** the Linux minimum RTO, the initial congestion window, and the loss-recovery algorithms.
- **F5-05:** the default socket buffer sizes and autotuning, keepalive timings, and `listen` backlog limits.
- **F5-06:**
  - the nsswitch lookup order, resolver time-outs and retries, and local caching services;
  - that DHCP clients use packet sockets, and what home routers run;
  - the 169.254/16 link-local fallback.
- **F5-07:** the Linux delayed-ACK timer values, "quick ACK" at connection start, and Nagle's exact release rule. The capture shows the effect but not the rule.

All external sources are cited by title only and were not opened during this build (dossier gate G1 open). These include:

- RFCs 768, 791, 792, 793, 826, 896, 1034, 1035, 1071, 1122, 1918, 2018, 2131, 2132, 2606, 3021, 3927, 4033, 4632, 4861, 5681, 5737, 6298, 6891, 7323, 8200 and 9293;
- Stevens and Fall, "TCP/IP Illustrated, Volume 1";
- Stevens, Fenner and Rudoff, "UNIX Network Programming, Volume 1";
- Gregg, "Systems Performance";
- IEEE 802.3;
- POSIX;
- the Linux man pages;
- the C++20 standard;
- the FNV hash description. The FNV-1a constants in F5-05 were written from memory; the lab only tests that sender and receiver agree.

The Systems Curriculum (item 50, milestone C9) was opened as a local file.

## Analogy mappings proposed (F5: "friends in different towns")

These are new and need registering. Each one is also listed in its chapter's meta comment.

- **F5-01:**
  - packet = postcard or envelope
  - header = outside of the envelope
  - payload = the letter
  - checksum = the letter count written at the bottom
  - loss, duplication and reordering = the post's mishaps
- **F5-02:**
  - layers = envelope inside a mail bag inside a van
  - port = the person's name within a house
  - demultiplexing = the sorting at each step
- **F5-03:**
  - MAC address = the name on the mailbox
  - IPv4 address = the postal address
  - prefix = the street
  - subnet mask = the "is this my street?" rule
  - ARP = calling out in the street
  - default gateway = the street's post office
  - router = sorting office
- **F5-04:**
  - UDP = postcards
  - TCP = a numbered letter series with receipts
  - receive window = mailbox space
  - three-way handshake = "shall we write?" / "yes, shall we?" / "yes"
- **F5-05:**
  - socket = a mailbox slot owned by one program
  - listening socket = a receptionist who hands each caller a desk (`accept`)
  - kernel socket buffers = the outgoing and incoming trays of the post room
- **F5-06:**
  - DNS = the town's address book
  - authoritative server = the town hall that keeps its own book
  - TTL = "good for five minutes"
  - DHCP = the town hall that lends a house number (the lease)
- **F5-07:**
  - latency = how long one letter takes
  - throughput = pages per day
  - bandwidth-delay product = how many pages must be on the road at once
  - Nagle's algorithm = the clerk who holds a small note until the last receipt returns
  - delayed ACK = the receipt held back until there is a reply

## Glossary

`glossary.json` has 53 new entries, taken from the chapters' jargon boxes.

- **Reused exact terms (owner decision 7):** the build keeps the earlier definitions of these, which are not duplicated in this course:
  - Packet (KID101)
  - Router (KID101)
  - MAC address (HW205)
  - Endianness (byte order) (SP201)
  - File descriptor (SP102)
  - RAII (SP102)
  - Median and percentile (SP302 / MA202)
  - Little's law (HW301)
  - Warm-up run (SP302)
- **Qualified terms, to avoid clashing with general terms from other courses:**
  - Latency (network)
  - Throughput (network)
  - Header (packet)
  - Frame (link layer)
  - Segment (TCP)
  - Port (transport)
  - Broadcast (Ethernet)
  - TTL (DNS time to live)

## Decisions for the owner

1. **Root and namespaces.** The real network evidence (captures, `iptables` loss, the private `resolv.conf`) needs root and `unshare`. Is that acceptable for the reproducible-build contract? A non-root fallback is logged as "untested in this environment".
2. **Capture files are kept in the lab folders** (`.pcap` and `.pcapng`, a few KB each) so learners can open them in Wireshark. F5-02's `kernel_decode` reads `../F5-01/send_one.pcapng`, so F5-01 must run before F5-02.
3. **The pcap writer is from memory.** The classic pcap header and record layout in F5-02 and F5-03 and in `dhcp_frames.cpp` are checked only by tshark accepting the files. The Source Researcher should confirm the format document.
4. **Simulated frames.** The forensic evidence in F5-03 (ARP) and the DHCP exchange in F5-06 are frames built by our own programs, not captures of real networks. Both chapters say so and carry "untested on hardware" boxes. Approve, or schedule a real-LAN capture.
5. **Run-dependent numbers.** Loopback timings, TCP port numbers, initial sequence numbers, DNS query IDs and retransmission counts change on every run. The prose quotes the final recorded run, says "in the recorded run" where needed, and avoids numbers that the rendered output does not show. A rebuild will need these quotes refreshed: F5-04 (Figure 1 port, raw ISN, retransmission and zero-window counts), F5-05 (recv count, truncated sizes, 100 MiB time) and F5-07 (all measurements).
6. **Word counts.** Chapters run about 4,500–5,500 words of prose, plus tables. This is above a short-chapter target, because each chapter carries the full template plus captures. Trim if the owner prefers.
7. **Glossary reuse.** Exact-term reuse is described above. Please confirm that the earlier definitions of Packet, Router and MAC address (from a KID course and a hardware course) are acceptable for L2 networking readers.
8. **Zone name.** The F5-06 lab zone uses `web.lab.example` instead of a name that begins with "www.", so that no text in the course looks like a URL.
