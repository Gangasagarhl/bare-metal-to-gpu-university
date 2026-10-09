// minicc.cpp - F3-55: a very small compiler, used to show the bootstrap comparison idea.
// Language: one statement per line, "let NAME = EXPR" or "print EXPR"; EXPR uses integers,
// variables (one lower-case letter), + - * and parentheses. Output: x86-64 assembly (GNU as,
// AT&T syntax) for a static Linux program that needs no C library. Reads stdin, writes stdout.
#include <cctype>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

class Parser
{
public:
    Parser(const std::string& text, std::map<char, int>& vars, std::ostream& out)
        : s_(text), vars_(vars), out_(out) {}

    void expr()                                   // expr := term { (+|-) term }
    {
        term();
        while (peek() == '+' || peek() == '-') {
            char op = s_[pos_++];
            term();
            out_ << "  pop %rcx\n  pop %rax\n"
                 << (op == '+' ? "  add %rcx, %rax\n" : "  sub %rcx, %rax\n") << "  push %rax\n";
        }
    }

private:
    void term()                                   // term := factor { * factor }
    {
        factor();
        while (peek() == '*') {
            ++pos_;
            factor();
            out_ << "  pop %rcx\n  pop %rax\n  imul %rcx, %rax\n  push %rax\n";
        }
    }

    void factor()                                 // factor := number | variable | ( expr )
    {
        char c = peek();
        if (c == '(') {
            ++pos_;
            expr();
            if (peek() != ')') throw std::runtime_error("expected )");
            ++pos_;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            long v = 0;
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_]))) {
                v = v * 10 + (s_[pos_++] - '0');
            }
            out_ << "  push $" << v << "\n";
        } else if (std::islower(static_cast<unsigned char>(c)) && vars_.count(c) != 0) {
            ++pos_;
            out_ << "  push var_" << c << "(%rip)\n";
        } else {
            throw std::runtime_error(std::string("unexpected '") + c + "'");
        }
    }

    char peek()
    {
        while (pos_ < s_.size() && s_[pos_] == ' ') ++pos_;
        return pos_ < s_.size() ? s_[pos_] : '\0';
    }

    std::string s_;
    std::size_t pos_ = 0;
    std::map<char, int>& vars_;
    std::ostream& out_;
};

// The runtime: print the signed number in %rax and a newline with the write system call.
const char* runtime = R"(print_rax:
  sub $32, %rsp
  lea 31(%rsp), %rsi
  movb $10, (%rsi)
  mov $1, %r8
  mov %rax, %r9
  test %rax, %rax
  jns 1f
  neg %rax
1:
  mov $10, %rcx
2:
  xor %rdx, %rdx
  div %rcx
  add $48, %dl
  dec %rsi
  mov %dl, (%rsi)
  inc %r8
  test %rax, %rax
  jnz 2b
  test %r9, %r9
  jns 3f
  dec %rsi
  movb $45, (%rsi)
  inc %r8
3:
  mov $1, %eax
  mov $1, %edi
  mov %r8, %rdx
  syscall
  add $32, %rsp
  ret
)";

} // namespace

int main()
{
    std::map<char, int> vars;
    std::ostringstream code;
    std::string line;
    int n = 0;
    try {
        while (std::getline(std::cin, line)) {
            ++n;
            if (line.empty() || line[0] == '#') continue;
            if (line.rfind("let ", 0) == 0 && line.size() > 8 && line[5] == ' ' && line[6] == '=') {
                char name = line[4];
                Parser p(line.substr(7), vars, code);
                p.expr();
                vars[name] = 1;
                code << "  pop %rax\n  mov %rax, var_" << name << "(%rip)\n";
            } else if (line.rfind("print ", 0) == 0) {
                Parser p(line.substr(6), vars, code);
                p.expr();
                code << "  pop %rax\n  call print_rax\n";
            } else {
                throw std::runtime_error("unknown statement");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "minicc: line " << n << ": " << e.what() << '\n';
        return 1;
    }
    std::cout << "  .text\n  .globl _start\n_start:\n" << code.str()
              << "  mov $60, %eax\n  xor %edi, %edi\n  syscall\n" << runtime << "  .bss\n";
    for (const auto& [name, used] : vars) {
        std::cout << "var_" << name << ": .quad 0\n";
    }
    return 0;
}
