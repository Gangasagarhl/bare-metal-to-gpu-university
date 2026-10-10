# MP4 Listing 10: one resource-and-instruction table per build target, from the compiler's own
# output (AMD: hipcc -S assembly with its metadata; NVIDIA: cuobjdump -sass text).
#   python3 -I isa_summary.py amd <target> <file.s>
#   python3 -I isa_summary.py nvidia <target> <file.sass>
# FMA count = FP32 FMAs, counting each half of a dual-issue pair (gfx11 "a :: b") separately and
# a packed v_pk_fma_f32 (gfx90a, gfx942) as two FP32 FMAs, as its name and operands say.
# MUL counts FP32 multiplies that were NOT fused (v_mul_f32, v_pk_mul_f32 = 2, FMUL): the
# compiler may keep a multiply and an add apart; FMA + MUL is the static multiply count.
import re
import sys

CFG = re.compile(r"TileCfgILi(\d+)ELi(\d+)ELi(\d+)ELi(\d+)ELi(\d+)ELi(\d+)E")


def short(mangled):
    m = CFG.search(mangled)
    return "BM%s BN%s BK%s TM%s TN%s" % m.groups()[:5] if m else mangled


def amd(target, path):
    text = open(path).read()
    bodies = re.findall(r"^(_Z\w+):.*?$(.*?)^\.Lfunc_end", text, re.S | re.M)
    meta = {}
    for entry in re.split(r"^  - ", text.split("amdhsa.kernels:")[1], flags=re.M)[1:]:
        fields = dict(re.findall(r"^(?:|    )\.(\w+):\s+(\S+)", entry, re.M))   # kernel level only
        if "name" in fields:
            meta[fields["name"]] = fields
    print("target %s" % target)
    print("  %-26s %5s %5s %5s %6s %5s %5s %5s %5s %5s %5s %5s" % (
        "instance", "wave", "VGPR", "SGPR", "LDS B", "spill", "FMA", "MUL", "LDSrd", "LDSwr", "bar", "mfma"))
    for name, body in bodies:
        ops = []
        for line in re.findall(r"^\s+([a-z].*)$", body, re.M):
            ops += [half.split()[0] for half in line.split("::") if half.strip()]
        fma = sum(1 for o in ops if re.match(r"v_(dual_)?fmac?_f32", o)) + 2 * sum(1 for o in ops if o.startswith("v_pk_fma_f32"))
        mul = sum(1 for o in ops if re.match(r"v_(dual_)?mul_f32", o)) + 2 * sum(1 for o in ops if o.startswith("v_pk_mul_f32"))
        rd = sum(1 for o in ops if re.match(r"ds_(read|load)", o))
        wr = sum(1 for o in ops if re.match(r"ds_(write|store)", o))
        bar = ops.count("s_barrier")
        mf = sum(1 for o in ops if o.startswith(("v_mfma", "v_wmma")))
        f = meta[name]
        print("  %-26s %5s %5s %5s %6s %5s %5d %5d %5d %5d %5d %5d" % (
            short(name), f.get("wavefront_size"), f.get("vgpr_count"), f.get("sgpr_count"),
            f.get("group_segment_fixed_size"), f.get("vgpr_spill_count"), fma, mul, rd, wr, bar, mf))


def nvidia(target, path):
    text = open(path).read()
    print("target %s" % target)
    print("  %-26s %5s %5s %5s %5s %5s %5s" % ("instance", "FFMA", "FMUL", "LDS", "STS", "BAR", "HMMA"))
    for name, body in re.findall(r"Function : (\S+)(.*?)(?=Function : |\Z)", text, re.S):
        ops = re.findall(r"/\*[0-9a-f]{4}\*/\s+(?:@!?P\d\s+)?([A-Z][A-Z0-9.]*)", body)
        cnt = lambda p: sum(1 for o in ops if o.split(".")[0] == p)
        print("  %-26s %5d %5d %5d %5d %5d %5d" % (short(name), cnt("FFMA"), cnt("FMUL"), cnt("LDS"), cnt("STS"), cnt("BAR"), cnt("HMMA")))


if __name__ == "__main__":
    (amd if sys.argv[1] == "amd" else nvidia)(sys.argv[2], sys.argv[3])
