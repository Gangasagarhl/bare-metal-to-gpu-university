// hitl_link.cpp - the HITL split, rehearsed on one machine.
// Thread "sim" owns the world (it would stay on the PC); thread "fc" owns the flight
// software (it would run on the flight controller). They talk ONLY through bytes on a
// socketpair, framed with this course's own small protocol (NOT MAVLink):
//   0xA5 | type | seq | len | payload (float32 little-endian) | CRC-16/CCITT-FALSE (2 bytes)
// Every 997th sensor frame is corrupted on purpose to exercise the CRC check.
#include "../F10-33/dronesim.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

std::uint16_t crc16(const std::uint8_t* d, std::size_t n)  // poly 0x1021, init 0xFFFF
{
    std::uint16_t c = 0xFFFF;
    for (std::size_t i = 0; i < n; ++i) {
        c ^= static_cast<std::uint16_t>(d[i] << 8);
        for (int b = 0; b < 8; ++b)
            c = (c & 0x8000) ? static_cast<std::uint16_t>((c << 1) ^ 0x1021)
                             : static_cast<std::uint16_t>(c << 1);
    }
    return c;
}

Bytes frame(std::uint8_t type, std::uint8_t seq, const std::vector<float>& v)
{
    Bytes b = {0xA5, type, seq, static_cast<std::uint8_t>(v.size() * 4)};
    for (float f : v) {
        std::array<std::uint8_t, 4> raw;
        std::memcpy(raw.data(), &f, 4);
        b.insert(b.end(), raw.begin(), raw.end());
    }
    const std::uint16_t c = crc16(b.data() + 1, b.size() - 1);
    b.push_back(static_cast<std::uint8_t>(c & 0xFF));
    b.push_back(static_cast<std::uint8_t>(c >> 8));
    return b;
}

bool read_all(int fd, std::uint8_t* p, std::size_t n)
{
    while (n > 0) {
        const ssize_t r = read(fd, p, n);
        if (r <= 0) return false;
        p += r;
        n -= static_cast<std::size_t>(r);
    }
    return true;
}

// Returns false on a CRC error (the bytes are consumed either way).
bool recv_frame(int fd, std::vector<float>& out, bool& link_ok)
{
    std::array<std::uint8_t, 4> h;
    link_ok = read_all(fd, h.data(), 4) && h[0] == 0xA5;
    if (!link_ok) return false;
    Bytes body(h[3] + 2u);
    link_ok = read_all(fd, body.data(), body.size());
    if (!link_ok) return false;
    Bytes all(h.begin() + 1, h.end());
    all.insert(all.end(), body.begin(), body.end() - 2);
    const std::uint16_t got =
        static_cast<std::uint16_t>(body[body.size() - 2] | (body.back() << 8));
    if (crc16(all.data(), all.size()) != got) return false;
    out.resize(h[3] / 4);
    std::memcpy(out.data(), body.data(), h[3]);
    return true;
}

void send_bytes(int fd, const Bytes& b)
{
    std::size_t off = 0;
    while (off < b.size()) {
        const ssize_t w = write(fd, b.data() + off, b.size() - off);
        if (w <= 0) return;
        off += static_cast<std::size_t>(w);
    }
}

struct Stats
{
    long frames = 0, bytes_sensor = 0, bytes_cmd = 0, crc_errors = 0;
};

int main()
{
    const char* t9 = "123456789";
    std::printf("CRC-16/CCITT-FALSE self-test of \"123456789\": 0x%04X\n",
                crc16(reinterpret_cast<const std::uint8_t*>(t9), 9));
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
        std::perror("socketpair");
        return 1;
    }
    Stats st;
    std::vector<dn::Event> fc_events;

    std::thread fc([&] {  // ---------- "flight controller" side
        dn::Fsw f;
        std::vector<float> v;
        std::array<float, 4> last_cmd = {0, 0, 0, 0};
        std::uint8_t seq = 0;
        bool armed_once = false, link_ok = true;
        for (;;) {
            const bool good = recv_frame(sv[1], v, link_ok);
            if (!link_ok) break;
            if (!good) {
                ++st.crc_errors;  // reject the frame; repeat the last command, marked stale
                send_bytes(sv[1], frame(2, seq++, {last_cmd[0], last_cmd[1], last_cmd[2], 2.0f}));
                continue;
            }
            dn::Sensors s;
            s.t = v[0];
            s.gps_pos = {v[1], v[2], v[3]};
            s.gps_vel = {v[4], v[5], v[6]};
            s.mag_heading = v[7];
            s.mag_norm = v[8];
            s.volts = v[9];
            s.amps = v[10];
            s.rc_valid = v[11] > 0.5f;
            s.rc_lq = static_cast<int>(v[12]);
            if (!armed_once) {
                f.arm(s, {{20, 0, 10}, {20, 20, 10}, {0, 20, 10}});
                armed_once = true;
            }
            const dn::Command c = f.step(s);
            last_cmd = {static_cast<float>(c.acc.x), static_cast<float>(c.acc.y),
                        static_cast<float>(c.acc.z), c.armed ? 1.0f : 0.0f};
            send_bytes(sv[1],
                       frame(2, seq++, {last_cmd[0], last_cmd[1], last_cmd[2], last_cmd[3]}));
        }
        fc_events = f.events;
    });

    // ---------- "simulator" side (this thread)
    dn::World w;
    w.wind = {1.0, 0.5, 0};
    std::uint8_t seq = 0;
    std::vector<float> cmd;
    bool link_ok = true, was_armed = false;
    while (w.t < 300.0) {
        const dn::Sensors s = w.sense();
        Bytes b = frame(1, seq++,
                        {static_cast<float>(s.t), static_cast<float>(s.gps_pos.x),
                         static_cast<float>(s.gps_pos.y), static_cast<float>(s.gps_pos.z),
                         static_cast<float>(s.gps_vel.x), static_cast<float>(s.gps_vel.y),
                         static_cast<float>(s.gps_vel.z), static_cast<float>(s.mag_heading),
                         static_cast<float>(s.mag_norm), static_cast<float>(s.volts),
                         static_cast<float>(s.amps), s.rc_valid ? 1.0f : 0.0f,
                         static_cast<float>(s.rc_lq)});
        if (st.frames % 997 == 996) b[10] ^= 0x40;  // injected bit error
        st.bytes_sensor += static_cast<long>(b.size());
        send_bytes(sv[0], b);
        if (!recv_frame(sv[0], cmd, link_ok)) {
            std::printf("command frame lost\n");
            break;
        }
        st.bytes_cmd += 4 + 16 + 2;
        ++st.frames;
        dn::Command c;
        c.acc = {cmd[0], cmd[1], cmd[2]};
        c.armed = cmd[3] > 0.5f;
        if (cmd[3] > 1.5f) c.armed = was_armed;  // stale repeat: keep the arming state
        was_armed = c.armed;
        w.step(c);
        if ((!c.armed && w.t > 1.0) || w.crashed) break;
    }
    shutdown(sv[0], SHUT_RDWR);
    fc.join();
    close(sv[0]);
    close(sv[1]);

    for (const auto& e : fc_events) std::printf("fc event %7.2f s  %s\n", e.t, e.text.c_str());
    std::printf("frames exchanged        %ld (one per 10 ms step)\n", st.frames);
    std::printf("sensor frame size       %ld bytes, command frame size 22 bytes\n",
                st.bytes_sensor / st.frames);
    std::printf("CRC errors detected     %ld (frames corrupted on purpose: %ld)\n", st.crc_errors,
                st.frames / 997);
    const double bps = (st.bytes_sensor / static_cast<double>(st.frames)) * 100.0;
    std::printf("sensor stream needs     %.0f bytes/s = %.0f bit/s at 10 bits per byte\n", bps,
                bps * 10);
    std::printf("landing error           %.3f m, crashed: %s\n", dn::hnorm(w.pos),
                w.crashed ? "yes" : "no");
    return 0;
}
