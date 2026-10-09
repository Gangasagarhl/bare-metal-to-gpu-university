// aml.h - DR302 F4-11: a namespace walker for ACPI Machine Language (AML).
// It reads the definition blocks (DSDT, SSDT) and reports every named object it can see
// without running code: Scope, Device, Method, Name, OperationRegion, Field units and a few
// more. It does NOT execute methods; anything a method computes stays invisible. A real
// kernel needs a full interpreter (for example ACPICA) for that.
// Freestanding: no library calls, so the same file builds into the kernel and the host tool.
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace aml {
enum class Kind : uint8_t { Scope, Device, Method, Name, Region, FieldUnit, Processor, PowerResource,
                            ThermalZone, Mutex, Event, Alias, External };
const char* kind_name(Kind k);

struct Node {
    Kind kind;
    uint8_t depth;            // nesting depth below the root
    char path[96];            // absolute path, trailing '_' of each segment trimmed: \_SB.PCI0
    uint32_t offset;          // of the opcode, from the start of the table
    const uint8_t* obj;       // Name: its data object. Method: its flags byte. Region: space byte
    const uint8_t* end;       // where the enclosing object ends (a bound for parsing obj)
    uint32_t bit_offset;      // FieldUnit: position inside its region, in bits
    uint32_t bit_width;       // FieldUnit: width in bits
    char region[96];          // FieldUnit: the region (or index register) it belongs to
};
using Visit = void (*)(const Node& n, void* ctx);

struct Result {
    uint32_t nodes;           // named objects reported
    uint32_t skipped;         // If/Else/While blocks at definition level, not looked into
    bool complete;            // false: stopped at an opcode this walker does not know
    uint32_t stop_offset;
    uint8_t stop_opcode;
};
// 'table' is a whole definition block, 36-byte header included.
Result walk(const uint8_t* table, uint32_t length, Visit visit, void* ctx);
// First node whose path equals 'path' (same trimmed form, for example "\\_S5").
bool find(const uint8_t* table, uint32_t length, const char* path, Node& out);

// Data objects (the right-hand side of Name(), package elements).
enum class T : uint8_t { Int, String, Buffer, Package, NameRef, Unsupported };
struct Value {
    T t;
    uint64_t i;               // Int
    const uint8_t* p;         // String: the text. Buffer: the bytes. Package: the first element
    uint32_t n;               // Buffer: byte count. Package: NumElements
    const uint8_t* end;       // Package: where it ends. Buffer: end of the bytes
    char name[96];            // NameRef: the name exactly as written (relative or absolute)
    uint8_t opcode;           // Unsupported: the opcode found
};
// Decodes one data object at p (not past 'end') and moves p behind it.
bool value(const uint8_t*& p, const uint8_t* end, Value& v);

// Resource templates (the Buffer that _CRS / _PRS return): the first interrupt in it.
// flags: bit 0 consumer, bit 1 edge (else level), bit 2 active low (else high), bit 3 shared.
bool first_interrupt(const Value& buffer, uint32_t& irq, uint8_t& flags);
}
