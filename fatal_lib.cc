// Replacement for gpsbabel's fatal.cc that throws instead of calling exit().
// Allows GdbFormat::read() failures to be caught by the library wrapper.

#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <string>

#include "defs.h"
#include "src/core/logging.h"

namespace {

std::string vformat(const char* fmt, va_list ap) {
    va_list ap2;
    va_copy(ap2, ap);
    int n = std::vsnprintf(nullptr, 0, fmt, ap2);
    va_end(ap2);
    if (n < 0) {
        return "fatal: <format error>";
    }
    std::string out(static_cast<std::size_t>(n) + 1, '\0');
    std::vsnprintf(out.data(), out.size(), fmt, ap);
    out.resize(static_cast<std::size_t>(n));
    return out;
}

} // namespace

[[noreturn]] void fatal(QDebug& msginstance) {
    auto* myinstance = new FatalMsg;
    myinstance->swap(msginstance);
    delete myinstance;
    throw std::runtime_error("gpsbabel fatal");
}

[[noreturn]] void fatal(const char* fmt, ...) {
    std::fflush(stdout);
    va_list ap;
    va_start(ap, fmt);
    std::string msg = vformat(fmt, ap);
    va_end(ap);
    throw std::runtime_error(msg);
}

void warning(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
}
