// aml.cc - DR302 F4-11: the AML namespace walker (see aml.h). No library calls.
#include "aml.h"

namespace aml {
namespace {
constexpr uint8_t ZERO = 0x00, ONE = 0x01, ALIAS = 0x06, NAME = 0x08, BYTE = 0x0A, WORD = 0x0B, DWORD = 0x0C,
                  STRING = 0x0D, QWORD = 0x0E, SCOPE = 0x10, BUFFER = 0x11, PACKAGE = 0x12, VARPACKAGE = 0x13,
                  METHOD = 0x14, EXTERNAL = 0x15, EXTOP = 0x5B, ROOT = 0x5C, PARENT = 0x5E, DUAL = 0x2E,
                  MULTI = 0x2F, IF = 0xA0, ELSE = 0xA1, WHILE = 0xA2, ONES = 0xFF;
// second byte after EXTOP (0x5B)
constexpr uint8_t X_MUTEX = 0x01, X_EVENT = 0x02, X_REGION = 0x80, X_FIELD = 0x81, X_DEVICE = 0x82,
                  X_PROCESSOR = 0x83, X_POWERRES = 0x84, X_THERMAL = 0x85, X_INDEXFIELD = 0x86,
                  X_BANKFIELD = 0x87;

size_t slen(const char* s) { size_t n = 0; while (s[n]) ++n; return n; }
bool seq(const char* a, const char* b) { while (*a && *a == *b) { ++a; ++b; } return *a == *b; }
void scopy(char* d, const char* s, size_t cap) { size_t i = 0; for (; s[i] && i + 1 < cap; ++i) d[i] = s[i]; d[i] = 0; }

bool lead_char(uint8_t c) { return (c >= 'A' && c <= 'Z') || c == '_'; }

// PkgLength: bits 7-6 of the lead byte = number of extra bytes (0-3). With none, bits 5-0
// are the length; otherwise bits 3-0 are the low nibble and each extra byte adds 8 bits.
// The length counts from the first PkgLength byte. 'raw' returns the number itself.
bool pkg_raw(const uint8_t*& p, const uint8_t* end, uint32_t& raw)
{
    if (p >= end) return false;
    const uint8_t lead = *p++;
    const uint8_t extra = lead >> 6;
    if (p + extra > end) return false;
    if (extra == 0) { raw = lead & 0x3F; return true; }
    raw = lead & 0x0F;
    for (uint8_t k = 0; k < extra; ++k) raw |= uint32_t{p[k]} << (4 + 8 * k);
    p += extra;
    return true;
}
bool pkg_length(const uint8_t*& p, const uint8_t* end, const uint8_t*& pkg_end)
{
    const uint8_t* start = p;
    uint32_t raw;
    if (!pkg_raw(p, end, raw)) return false;
    pkg_end = start + raw;
    return pkg_end <= end && pkg_end >= p;
}

// Appends one 4-character segment to 'out' as ".ABCD" with trailing '_' trimmed.
void add_seg(char* out, size_t cap, const uint8_t* s)
{
    size_t n = slen(out);
    if (n > 1 && n + 1 < cap) out[n++] = '.';          // "\" alone gets no dot
    int w = 4;
    while (w > 1 && s[w - 1] == '_') --w;
    for (int k = 0; k < w && n + 1 < cap; ++k) out[n++] = static_cast<char>(s[k]);
    out[n] = 0;
}
void drop_seg(char* path)
{
    size_t n = slen(path);
    while (n > 1 && path[n - 1] != '.' && path[n - 1] != '\\') --n;
    if (n > 1 && path[n - 1] == '.') --n;
    path[n] = 0;
}

// NameString: optional '\' or '^'s, then NullName / one segment / DualNamePrefix /
// MultiNamePrefix. 'abs' gets the absolute path (relative names hang below 'scope');
// 'raw' gets the name as written.
bool name_string(const uint8_t*& p, const uint8_t* end, const char* scope, char* abs, char* raw, size_t cap)
{
    abs[0] = 0;
    raw[0] = 0;
    size_t rn = 0;
    if (p >= end) return false;
    if (*p == ROOT) {
        scopy(abs, "\\", cap);
        raw[rn++] = '\\';
        ++p;
    } else {
        scopy(abs, scope, cap);
        while (p < end && *p == PARENT) { drop_seg(abs); raw[rn++] = '^'; ++p; }
    }
    raw[rn] = 0;
    if (p >= end) return false;
    uint32_t segs;
    if (*p == ZERO) { ++p; return true; }
    if (*p == DUAL) { segs = 2; ++p; }
    else if (*p == MULTI) { if (p + 1 >= end) return false; segs = p[1]; p += 2; }
    else segs = 1;
    if (p + 4 * segs > end) return false;
    for (uint32_t s = 0; s < segs; ++s, p += 4) {
        if (!lead_char(p[0])) return false;
        add_seg(abs, cap, p);
        size_t n = slen(raw);
        if (s > 0 && n + 1 < cap) raw[n++] = '.';
        raw[n] = 0;
        int w = 4;
        while (w > 1 && p[w - 1] == '_') --w;
        for (int k = 0; k < w && n + 1 < cap; ++k) raw[n++] = static_cast<char>(p[k]);
        raw[n] = 0;
    }
    return true;
}

struct Walker {
    const uint8_t* base;
    Visit visit;
    void* ctx;
    Result r;
};

bool emit(Walker& w, Kind k, const char* path, uint8_t depth, const uint8_t* op, const uint8_t* obj,
          const uint8_t* end, Node& n)
{
    n.kind = k;
    n.depth = depth;
    scopy(n.path, path, sizeof n.path);
    n.offset = static_cast<uint32_t>(op - w.base);
    n.obj = obj;
    n.end = end;
    n.bit_offset = n.bit_width = 0;
    n.region[0] = 0;
    ++w.r.nodes;
    if (w.visit) w.visit(n, w.ctx);
    return true;
}

bool stop(Walker& w, const uint8_t* p)
{
    w.r.complete = false;
    w.r.stop_offset = static_cast<uint32_t>(p - w.base);
    w.r.stop_opcode = *p;
    return false;
}

// The FieldList of Field / IndexField / BankField: named units with their bit positions.
bool field_list(Walker& w, const uint8_t* p, const uint8_t* end, const char* scope, const char* region,
                uint8_t depth, const uint8_t* op)
{
    uint32_t bit = 0;
    while (p < end) {
        if (*p == 0x00) {                              // ReservedField: skip bits
            ++p;
            uint32_t bits;
            if (!pkg_raw(p, end, bits)) return false;
            bit += bits;
        } else if (*p == 0x01) {                       // AccessField: access type, attribute
            p += 3;
        } else if (*p == 0x03) {                       // ExtendedAccessField
            p += 4;
        } else if (lead_char(*p)) {                    // NamedField: NameSeg, then width in bits
            char path[96];
            scopy(path, scope, sizeof path);
            add_seg(path, sizeof path, p);
            const uint8_t* seg = p;
            p += 4;
            uint32_t bits;
            if (!pkg_raw(p, end, bits)) return false;
            Node n;
            n.kind = Kind::FieldUnit;
            n.depth = depth;
            scopy(n.path, path, sizeof n.path);
            n.offset = static_cast<uint32_t>(op - w.base);
            n.obj = seg;
            n.end = end;
            n.bit_offset = bit;
            n.bit_width = bits;
            scopy(n.region, region, sizeof n.region);
            ++w.r.nodes;
            if (w.visit) w.visit(n, w.ctx);
            bit += bits;
        } else {
            return false;                              // ConnectField and others: not handled
        }
    }
    return true;
}

bool term_list(Walker& w, const uint8_t* p, const uint8_t* end, char* scope, uint8_t depth);

// One named object with a body (Scope, Device, ...): walk the body with the new scope.
bool nested(Walker& w, const uint8_t* body, const uint8_t* end, const char* path, uint8_t depth)
{
    char inner[96];
    scopy(inner, path, sizeof inner);
    return term_list(w, body, end, inner, static_cast<uint8_t>(depth + 1));
}

bool term_list(Walker& w, const uint8_t* p, const uint8_t* end, char* scope, uint8_t depth)
{
    char path[96], raw[96];
    Node n;
    while (p < end) {
        const uint8_t* op = p;
        const uint8_t c = *p++;
        const uint8_t* pend;
        if (c == SCOPE || c == METHOD) {
            if (!pkg_length(p, end, pend) || !name_string(p, pend, scope, path, raw, sizeof path)) return stop(w, op);
            if (c == SCOPE) {
                emit(w, Kind::Scope, path, depth, op, p, pend, n);
                if (!nested(w, p, pend, path, depth)) return false;
            } else {
                emit(w, Kind::Method, path, depth, op, p, pend, n);   // obj = MethodFlags
            }
            p = pend;
        } else if (c == NAME) {
            if (!name_string(p, end, scope, path, raw, sizeof path)) return stop(w, op);
            const uint8_t* obj = p;
            Value v;
            if (!value(p, end, v) || v.t == T::Unsupported) return stop(w, op);
            emit(w, Kind::Name, path, depth, op, obj, end, n);
        } else if (c == ALIAS) {
            char target[96];
            if (!name_string(p, end, scope, target, raw, sizeof target)) return stop(w, op);
            if (!name_string(p, end, scope, path, raw, sizeof path)) return stop(w, op);
            emit(w, Kind::Alias, path, depth, op, nullptr, end, n);
        } else if (c == EXTERNAL) {
            if (!name_string(p, end, scope, path, raw, sizeof path) || p + 2 > end) return stop(w, op);
            p += 2;                                    // ObjectType, ArgumentCount
            emit(w, Kind::External, path, depth, op, nullptr, end, n);
        } else if (c == IF || c == ELSE || c == WHILE) {
            if (!pkg_length(p, end, pend)) return stop(w, op);
            ++w.r.skipped;                             // code: only an interpreter can decide it
            p = pend;
        } else if (c == EXTOP && p < end) {
            const uint8_t x = *p++;
            if (x == X_DEVICE || x == X_THERMAL || x == X_PROCESSOR || x == X_POWERRES) {
                if (!pkg_length(p, end, pend) || !name_string(p, pend, scope, path, raw, sizeof path)) return stop(w, op);
                Kind k = x == X_DEVICE ? Kind::Device : x == X_THERMAL ? Kind::ThermalZone
                       : x == X_PROCESSOR ? Kind::Processor : Kind::PowerResource;
                if (x == X_PROCESSOR) p += 6;          // ProcID, PblkAddr (4), PblkLen
                if (x == X_POWERRES) p += 3;           // SystemLevel, ResourceOrder (2)
                emit(w, k, path, depth, op, p, pend, n);
                if (!nested(w, p, pend, path, depth)) return false;
                p = pend;
            } else if (x == X_REGION) {
                if (!name_string(p, end, scope, path, raw, sizeof path) || p >= end) return stop(w, op);
                const uint8_t* obj = p;
                ++p;                                   // RegionSpace
                Value off, len;
                if (!value(p, end, off) || off.t != T::Int || !value(p, end, len) || len.t != T::Int) return stop(w, op);
                emit(w, Kind::Region, path, depth, op, obj, p, n);
            } else if (x == X_FIELD || x == X_INDEXFIELD || x == X_BANKFIELD) {
                char region[96];
                if (!pkg_length(p, end, pend) || !name_string(p, pend, scope, region, raw, sizeof region)) return stop(w, op);
                if (x == X_INDEXFIELD || x == X_BANKFIELD) {   // second name: data / bank register
                    if (!name_string(p, pend, scope, path, raw, sizeof path)) return stop(w, op);
                }
                if (x == X_BANKFIELD) {
                    Value bank;
                    if (!value(p, pend, bank)) return stop(w, op);
                }
                ++p;                                   // FieldFlags
                if (!field_list(w, p, pend, scope, region, depth, op)) return stop(w, op);
                p = pend;
            } else if (x == X_MUTEX) {
                if (!name_string(p, end, scope, path, raw, sizeof path)) return stop(w, op);
                ++p;                                   // SyncFlags
                emit(w, Kind::Mutex, path, depth, op, nullptr, end, n);
            } else if (x == X_EVENT) {
                if (!name_string(p, end, scope, path, raw, sizeof path)) return stop(w, op);
                emit(w, Kind::Event, path, depth, op, nullptr, end, n);
            } else {
                return stop(w, op);
            }
        } else {
            return stop(w, op);                        // code at definition level, or unknown
        }
    }
    return true;
}

struct FindCtx { const char* want; Node* out; bool found; };
void find_visit(const Node& n, void* ctx)
{
    auto* f = static_cast<FindCtx*>(ctx);
    if (!f->found && seq(n.path, f->want)) { *f->out = n; f->found = true; }
}
}  // namespace

const char* kind_name(Kind k)
{
    static const char* const names[] = {"Scope", "Device", "Method", "Name", "OperationRegion", "Field",
                                        "Processor", "PowerResource", "ThermalZone", "Mutex", "Event", "Alias",
                                        "External"};
    return names[static_cast<int>(k)];
}

bool value(const uint8_t*& p, const uint8_t* end, Value& v)
{
    v.t = T::Unsupported;
    v.i = 0;
    v.p = nullptr;
    v.n = 0;
    v.end = end;
    v.name[0] = 0;
    if (p >= end) return false;
    const uint8_t* op = p;
    const uint8_t c = *p++;
    v.opcode = c;
    auto le = [&](int bytes) {
        if (p + bytes > end) return false;
        v.i = 0;
        for (int k = 0; k < bytes; ++k) v.i |= uint64_t{p[k]} << (8 * k);
        p += bytes;
        v.t = T::Int;
        return true;
    };
    switch (c) {
    case ZERO: v.t = T::Int; v.i = 0; return true;
    case ONE: v.t = T::Int; v.i = 1; return true;
    case ONES: v.t = T::Int; v.i = ~uint64_t{0}; return true;
    case BYTE: return le(1);
    case WORD: return le(2);
    case DWORD: return le(4);
    case QWORD: return le(8);
    case STRING:
        v.t = T::String;
        v.p = p;
        while (p < end && *p) ++p;
        if (p >= end) return false;
        v.n = static_cast<uint32_t>(p - v.p);
        ++p;
        return true;
    case BUFFER: {
        const uint8_t* pend;
        if (!pkg_length(p, end, pend)) return false;
        Value size;
        if (!value(p, pend, size) || size.t != T::Int) return false;
        v.t = T::Buffer;
        v.p = p;
        v.n = static_cast<uint32_t>(pend - p);         // initialised bytes (size.i may be larger)
        v.end = pend;
        p = pend;
        return true;
    }
    case PACKAGE:
    case VARPACKAGE: {
        const uint8_t* pend;
        if (!pkg_length(p, end, pend)) return false;
        if (c == PACKAGE) {
            if (p >= pend) return false;
            v.n = *p++;
        } else {
            Value count;
            if (!value(p, pend, count) || count.t != T::Int) return false;
            v.n = static_cast<uint32_t>(count.i);
        }
        v.t = T::Package;
        v.p = p;
        v.end = pend;
        p = pend;
        return true;
    }
    default:
        if (c == ROOT || c == PARENT || c == DUAL || c == MULTI || lead_char(c)) {
            p = op;
            char abs[96];
            if (!name_string(p, end, "\\", abs, v.name, sizeof v.name)) return false;
            v.t = T::NameRef;
            return true;
        }
        p = op;
        return true;                                   // T::Unsupported: an expression, not data
    }
}

Result walk(const uint8_t* table, uint32_t length, Visit visit, void* ctx)
{
    Walker w{table, visit, ctx, Result{0, 0, true, 0, 0}};
    char scope[96] = "\\";
    if (length > 36) term_list(w, table + 36, table + length, scope, 0);
    return w.r;
}

bool find(const uint8_t* table, uint32_t length, const char* path, Node& out)
{
    FindCtx f{path, &out, false};
    walk(table, length, find_visit, &f);
    return f.found;
}

bool first_interrupt(const Value& buf, uint32_t& irq, uint8_t& flags)
{
    if (buf.t != T::Buffer) return false;
    const uint8_t* p = buf.p;
    const uint8_t* end = buf.end;
    while (p < end) {
        const uint8_t tag = *p;
        if (tag & 0x80) {                              // large item: tag, 16-bit length, data
            if (p + 3 > end) return false;
            const uint16_t len = static_cast<uint16_t>(p[1] | (p[2] << 8));
            const uint8_t* d = p + 3;
            if (d + len > end) return false;
            if ((tag & 0x7F) == 0x09 && len >= 6) {    // Extended Interrupt descriptor
                const uint8_t f = d[0];
                irq = d[2] | (d[3] << 8) | (d[4] << 16) | (uint32_t{d[5]} << 24);
                flags = static_cast<uint8_t>((f & 0x01) | (f & 0x02) | (f & 0x04) | (f & 0x08));
                return true;
            }
            p = d + len;
        } else {                                       // small item: type in bits 6-3, length 2-0
            const uint8_t type = (tag >> 3) & 0x0F, len = tag & 0x07;
            const uint8_t* d = p + 1;
            if (d + len > end) return false;
            if (type == 0x0F) return false;            // End Tag
            if (type == 0x04 && len >= 2) {            // IRQ descriptor: 16-bit mask, optional info byte
                const uint16_t mask = static_cast<uint16_t>(d[0] | (d[1] << 8));
                uint8_t info = len >= 3 ? d[2] : 0x01; // without the byte: edge, active high
                for (uint32_t b = 0; b < 16; ++b)
                    if (mask & (1u << b)) {
                        irq = b;
                        flags = static_cast<uint8_t>(0x01 | ((info & 0x01) ? 0x02 : 0) | ((info & 0x08) ? 0x04 : 0) |
                                                     ((info & 0x10) ? 0x08 : 0));
                        return true;
                    }
                return false;
            }
            p = d + len;
        }
    }
    return false;
}
}  // namespace aml
