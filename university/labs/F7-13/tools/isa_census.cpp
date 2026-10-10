// F7-13 Listing 2 (tools/isa_census.cpp): count instruction classes in AMDGPU assembly
// (the .s text that hipcc -S writes), per kernel and per loop. A loop is a label that a
// later branch jumps back to. Reads the assembly on standard input.
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Inst {
    std::string text;
    std::string op;
};

std::string trim(const std::string& s)
{
    const auto b = s.find_first_not_of(" \t");
    if (b == std::string::npos) {
        return "";
    }
    const auto e = s.find_last_not_of(" \t");
    return s.substr(b, e - b + 1);
}

std::string classify(const std::string& op, const std::string& text)
{
    if (text.find("Spill") != std::string::npos || text.find("Reload") != std::string::npos ||
        op.rfind("scratch_", 0) == 0) {
        return "scratch (spill)";
    }
    if (op == "s_waitcnt") return "wait (s_waitcnt)";
    if (op.rfind("s_cbranch", 0) == 0 || op == "s_branch") return "branch";
    if (op.rfind("s_load", 0) == 0 || op.rfind("s_buffer_load", 0) == 0) return "scalar memory (s_load)";
    if (op.rfind("global_", 0) == 0 || op.rfind("buffer_", 0) == 0 || op.rfind("flat_", 0) == 0) {
        return "vector memory (global/buffer/flat)";
    }
    if (op.rfind("ds_", 0) == 0) return "LDS and cross-lane (ds_)";
    if (op.find("exec") != std::string::npos || text.find("exec") != std::string::npos) return "EXEC mask change";
    if (op.rfind("v_fma", 0) == 0 || op.rfind("v_fmac", 0) == 0 || op.rfind("v_pk_fma", 0) == 0 ||
        op.rfind("v_mfma", 0) == 0) return "vector FMA / matrix";
    if (op.rfind("v_", 0) == 0) return "vector ALU (other v_)";
    if (op.rfind("s_", 0) == 0) return "scalar ALU (other s_)";
    return "other";
}

void report(const std::string& title, const std::vector<Inst>& body)
{
    std::map<std::string, int> count;
    for (const Inst& i : body) {
        ++count[classify(i.op, i.text)];
    }
    std::printf("%s: %zu instructions\n", title.c_str(), body.size());
    for (const auto& kv : count) {
        std::printf("    %-36s %4d\n", kv.first.c_str(), kv.second);
    }
}

int main()
{
    std::string line, kernel;
    std::vector<Inst> insts;
    std::map<std::string, std::size_t> labelAt;              // label -> index of next instruction
    auto finish = [&]() {
        if (kernel.empty()) {
            return;
        }
        report("kernel " + kernel, insts);
        for (std::size_t k = 0; k < insts.size(); ++k) {     // backward branches mark loops
            const std::string& op = insts[k].op;
            if (op.rfind("s_cbranch", 0) != 0 && op != "s_branch") {
                continue;
            }
            const std::string code = insts[k].text.substr(0, insts[k].text.find(';'));
            const std::string target = trim(code.substr(op.size()));
            const auto it = labelAt.find(target);
            if (it != labelAt.end() && it->second <= k) {
                std::vector<Inst> body(insts.begin() + static_cast<long>(it->second),
                                       insts.begin() + static_cast<long>(k) + 1);
                report("  loop " + target + " (body)", body);
            }
        }
        kernel.clear();
        insts.clear();
        labelAt.clear();
    };
    while (std::getline(std::cin, line)) {
        const std::string raw = trim(line);                    // keeps the compiler's comments
        const std::string t = trim(line.substr(0, line.find(';')));
        if (t.empty() || t[0] == '.') {
            if (t.size() > 1 && t[0] == '.' && t.back() == ':' && t.rfind(".LBB", 0) == 0) {
                labelAt[t.substr(0, t.size() - 1)] = insts.size();
            }
            continue;
        }
        if (t.back() == ':' && t.rfind("_Z", 0) == 0) {       // a kernel's entry label
            finish();
            kernel = t.substr(0, t.size() - 1);
            continue;
        }
        if (kernel.empty()) {
            continue;
        }
        const std::string op = t.substr(0, t.find_first_of(" \t"));
        insts.push_back({raw, op});
        if (op == "s_endpgm") {
            finish();
        }
    }
    finish();
    return 0;
}
