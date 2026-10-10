#!/usr/bin/env bash
# canon_readobj.sh FILE: reformat llvm-readobj's view of a PE/COFF file into the same
# "Key value" lines that pe_read prints (only reformatting; every value comes from llvm-readobj).
set -u -o pipefail
f="$1"
llvm-readobj --file-headers --sections --coff-basereloc "$f" | awk '
    function num(v,   m) { if (match(v, /\(0x[0-9A-Fa-f]+\)/)) return tolower(substr(v, RSTART + 1, RLENGTH - 2)); return v }
    /^ImageFileHeader/ { hdr = 1 }
    /^ImageOptionalHeader/ { opt = 1; hdr = 0 }
    /^Sections \[/ { hdr = 0; opt = 0 }
    /^DOSHeader/ { hdr = 0; opt = 0 }
    /^  Machine:/                { m = num($0); sub(/.*\(/, "", m); sub(/\).*/, "", m); printf "Format %s\nMachine %s\n", (img ? "image" : "object"), m }
    /^  SectionCount:/           { print "SectionCount " $2 }
    /^  TimeDateStamp:/          { t = $0; sub(/.*\(/, "", t); sub(/\).*/, "", t); print "TimeDateStamp " tolower(t) }
    /^  PointerToSymbolTable:/   { print "PointerToSymbolTable " $2 }
    /^  SymbolCount:/            { print "SymbolCount " $2 }
    /^  OptionalHeaderSize:/     { print "OptionalHeaderSize " $2 }
    hdr && /^  Characteristics \[/ { c = $0; sub(/.*\(/, "", c); sub(/\).*/, "", c); print "Characteristics " c }
    opt && /^  Magic:/           { print "Magic " $2 }
    /^  AddressOfEntryPoint:/    { print "AddressOfEntryPoint " $2 }
    /^  ImageBase:/              { print "ImageBase " $2 }
    /^  SectionAlignment:/       { print "SectionAlignment " $2 }
    /^  FileAlignment:/          { print "FileAlignment " $2 }
    /^  SizeOfImage:/            { print "SizeOfImage " $2 }
    /^  SizeOfHeaders:/          { print "SizeOfHeaders " $2 }
    /^  Subsystem:/              { s = $0; sub(/.*\(/, "", s); sub(/\).*/, "", s); print "Subsystem " tolower(s) }
    opt && /^  Characteristics \[/ { d = $0; sub(/.*\(/, "", d); sub(/\).*/, "", d); print "DllCharacteristics " tolower(d) }
    /^  NumberOfRvaAndSize:/     { print "NumberOfRvaAndSize " $2 }
    /^    BaseRelocationTableRVA:/  { print "BaseRelocationTableRVA " $2 }
    /^    BaseRelocationTableSize:/ { print "BaseRelocationTableSize " $2 }
    /^  Section \{/              { sec = 1; line = "" }
    sec && /^    Number:/        { line = "Section " $2 }
    sec && /^    Name:/          { line = line " Name " $2 }
    sec && /^    VirtualSize:/   { line = line " VirtualSize " $2 }
    sec && /^    VirtualAddress:/ { line = line " VirtualAddress " $2 }
    sec && /^    RawDataSize:/   { line = line " RawDataSize " $2 }
    sec && /^    PointerToRawData:/ { line = line " PointerToRawData " $2 }
    sec && /^    PointerToRelocations:/ { line = line " PointerToRelocations " $2 }
    sec && /^    RelocationCount:/ { line = line " RelocationCount " $2 }
    sec && /^    Characteristics \[/ { x = $0; sub(/.*\(/, "", x); sub(/\).*/, "", x); line = line " Characteristics " tolower(x) }
    sec && /^  \}/               { print line; sec = 0 }
    /^  Entry \{/                { ent = 1 }
    ent && /^    Type:/          { ty = ($2 == "DIR64") ? 10 : ($2 == "ABSOLUTE") ? 0 : $2 }
    ent && /^    Address:/       { if (ty != 0) print "BaseReloc Type " ty " Address " $2; ent = 0 }
' img=$(head -c 2 "$f" | grep -c MZ)
