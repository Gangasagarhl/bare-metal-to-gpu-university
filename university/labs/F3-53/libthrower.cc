// libthrower.cc - F3-53: a shared library whose function throws a C++ exception.
// The exception must travel out of this library's frames into the caller's frames.
#include <stdexcept>
#include <string>

void throw_from_library(int code)
{
    throw std::runtime_error("thrown inside libthrower.so, code " + std::to_string(code));
}
