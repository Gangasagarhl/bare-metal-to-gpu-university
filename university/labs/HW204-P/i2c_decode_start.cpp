// HW204 practical (P): "decode a captured I2C transaction".
// STARTING FILE handed to the candidate. Complete the five functions marked TODO; do not
// change anything else. Build and run with the course command (one line):
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined i2c_decode_start.cpp -o i2c_decode_start
//   ./i2c_decode_start < i2c_decode_start.in
// As handed out, the TODOs return placeholders: the program parses the capture but decodes
// nothing, and the self-check at the end reports failures (exit code 1). When all five are
// right it reports 0 failures and the decoded transactions appear.
//
// The input (i2c_decode_start.in) is a logic-analyser-style capture of the two I2C lines in
// the text format of F1-47 (Listing 1): rows of 64 samples, '-' high, '_' low, four samples
// per bit. It was recorded by the course's bus model (which stands for a real I2C bus and a
// sigrok/PulseView capture); every address and value in it is an exercise value.
// Rules you need (F1-47, Layer 2): START = SDA falls while SCL is high; STOP = SDA rises
// while SCL is high; a receiver samples SDA on each rising edge of SCL; nine clocks per byte
// (8 data bits MSB first, then the ACK bit: low = ACK, high = NACK); the first byte after a
// START is the 7-bit address followed by the R/W bit (1 = read).
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Sample { int scl; int sda; };

enum class Kind { Start, Stop, Byte };

struct Event
{
    Kind kind;
    int value;          // the 8 data bits (Byte only)
    bool ack;           // 9th bit low (Byte only)
    bool is_address;    // first byte after a START (Byte only)
};

// Given: read the capture rows ("  64 SCL ---_" / "     SDA -__-") from stdin.
std::vector<Sample> parse_capture(std::istream& in)
{
    std::vector<Sample> out;
    std::string line, scl_row;
    while (std::getline(in, line)) {
        const auto p = line.find("SCL ");
        const auto q = line.find("SDA ");
        if (p != std::string::npos) {
            scl_row = line.substr(p + 4);
        } else if (q != std::string::npos) {
            const std::string sda_row = line.substr(q + 4);
            for (std::size_t i = 0; i < scl_row.size() && i < sda_row.size(); ++i) {
                out.push_back({scl_row[i] == '-' ? 1 : 0, sda_row[i] == '-' ? 1 : 0});
            }
            scl_row.clear();
        }
    }
    return out;
}

// TODO 1 (F1-47): a START condition between two consecutive samples a -> b.
bool is_start(Sample a, Sample b)
{
    (void)a; (void)b;  // delete this line when you use them
    return false;      // TODO 1
}

// TODO 2 (F1-47): a STOP condition between two consecutive samples a -> b.
bool is_stop(Sample a, Sample b)
{
    (void)a; (void)b;  // delete this line when you use them
    return false;      // TODO 2
}

// TODO 3 (F1-47): the moment a receiver samples SDA: the rising edge of SCL.
bool is_sample_point(Sample a, Sample b)
{
    (void)a; (void)b;  // delete this line when you use them
    return false;      // TODO 3
}

// TODO 4 (F1-47): split an address byte into the 7-bit address and the direction.
void split_address(int byte, int& addr7, bool& read)
{
    addr7 = byte;      // TODO 4: this treats the whole byte as the address
    read = false;
}

struct Summary
{
    int transactions = 0;               // one per STOP
    std::vector<int> acked;             // 7-bit addresses that answered ACK (no repeats)
    std::vector<int> nacked;            // 7-bit addresses that did not answer (no repeats)
    int bytes_written = 0;              // data bytes sent by the controller after an acked address + W
    int bytes_read = 0;                 // data bytes sent by a target after an acked address + R
};

[[maybe_unused]] static void add_unique(std::vector<int>& v, int x)
{
    for (int y : v) if (y == x) return;
    v.push_back(x);
}

// TODO 5 (F1-47): summarise the event list.
Summary summarise(const std::vector<Event>& ev)
{
    Summary s;
    (void)ev;          // delete this line when you use it
    return s;          // TODO 5: count transactions, acked and nacked addresses, bytes written and read
}

// Given: the decoder. It only uses the functions above and groups nine bits per byte.
std::vector<Event> decode(const std::vector<Sample>& tr)
{
    std::vector<Event> ev;
    std::vector<int> bits;
    bool first = false;
    auto flush = [&]() {
        if (bits.size() == 9) {
            int v = 0;
            for (int i = 0; i < 8; ++i) v = (v << 1) | bits[i];
            ev.push_back({Kind::Byte, v, bits[8] == 0, first});
            first = false;
        }
        bits.clear();
    };
    for (std::size_t i = 1; i < tr.size(); ++i) {
        const Sample a = tr[i - 1], b = tr[i];
        if (is_start(a, b)) {
            flush();
            ev.push_back({Kind::Start, 0, false, false});
            first = true;
        } else if (is_stop(a, b)) {
            flush();
            ev.push_back({Kind::Stop, 0, false, false});
        } else if (is_sample_point(a, b)) {
            bits.push_back(b.sda);
            if (bits.size() == 9) flush();
        }
    }
    return ev;
}

// Given: print the events the way F1-47's decoder does.
void print_events(const std::vector<Event>& ev)
{
    for (const Event& e : ev) {
        if (e.kind == Kind::Start) std::printf("  START\n");
        else if (e.kind == Kind::Stop) std::printf("  STOP\n");
        else if (e.is_address) {
            int a; bool r;
            split_address(e.value, a, r);
            std::printf("  address 0x%02X %s  (byte 0x%02X)  %s\n", a, r ? "READ" : "WRITE", e.value,
                        e.ack ? "ACK" : "NACK");
        } else {
            std::printf("  data    0x%02X  %s\n", e.value, e.ack ? "ACK" : "NACK");
        }
    }
}

static void print_list(const char* label, const std::vector<int>& v)
{
    std::printf("%s", label);
    if (v.empty()) std::printf(" none");
    for (int a : v) std::printf(" 0x%02X", a);
}

// Given: a self-check on a tiny hand-built capture (START, 0x90 ACK, 0x05 ACK, STOP),
// independent of the input. 0 failures does not prove the exam capture is decoded
// right; it proves the five functions agree with F1-47's rules.
int self_check()
{
    int failures = 0;
    std::vector<Sample> t;
    auto push = [&](int scl, int sda) { t.push_back({scl, sda}); };
    push(1, 1); push(1, 0); push(0, 0);                    // START
    auto bit = [&](int sda) { push(0, sda); push(0, sda); push(1, sda); push(1, sda); };
    for (int b : {1, 0, 0, 1, 0, 0, 0, 0}) bit(b);         // 0x90
    bit(0);                                                // ACK
    for (int b : {0, 0, 0, 0, 0, 1, 0, 1}) bit(b);         // 0x05
    bit(0);                                                // ACK
    push(0, 0); push(1, 0); push(1, 1); push(1, 1);        // STOP
    const std::vector<Event> ev = decode(t);
    const bool shape = ev.size() == 4 && ev[0].kind == Kind::Start && ev[1].kind == Kind::Byte &&
                       ev[2].kind == Kind::Byte && ev[3].kind == Kind::Stop;
    if (!shape) { std::printf("self-check: expected START, byte, byte, STOP; got %zu events\n", ev.size()); ++failures; }
    if (shape && !(ev[1].value == 0x90 && ev[1].ack && ev[1].is_address)) { std::printf("self-check: first byte wrong\n"); ++failures; }
    if (shape && !(ev[2].value == 0x05 && ev[2].ack && !ev[2].is_address)) { std::printf("self-check: second byte wrong\n"); ++failures; }
    int a = -1; bool r = true;
    split_address(0xA1, a, r);
    if (!(a == 0x50 && r)) { std::printf("self-check: split_address(0xA1) should give 0x50, read\n"); ++failures; }
    split_address(0x90, a, r);
    if (!(a == 0x48 && !r)) { std::printf("self-check: split_address(0x90) should give 0x48, write\n"); ++failures; }
    if (shape) {
        const Summary s = summarise(ev);
        if (!(s.transactions == 1 && s.acked.size() == 1 && s.acked[0] == 0x48 && s.nacked.empty() &&
              s.bytes_written == 1 && s.bytes_read == 0)) {
            std::printf("self-check: summary of the tiny capture wrong\n"); ++failures;
        }
    }
    return failures;
}

int main()
{
    const std::vector<Sample> tr = parse_capture(std::cin);
    std::printf("capture: %zu samples\n", tr.size());
    const std::vector<Event> ev = decode(tr);
    std::printf("decoded from the capture:\n");
    print_events(ev);
    const Summary s = summarise(ev);
    std::printf("summary: transactions %d;", s.transactions);
    print_list(" addresses acknowledged:", s.acked);
    print_list("; addresses not acknowledged:", s.nacked);
    std::printf("; data bytes written %d, read %d\n", s.bytes_written, s.bytes_read);
    const int f = self_check();
    std::printf("self-check: %d failures\n", f);
    return f == 0 ? 0 : 1;
}
