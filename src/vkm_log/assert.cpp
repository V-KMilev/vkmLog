#include "vkm_log/assert.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace Vkm::Log {

namespace {

/// @p fmt with @p args, whatever its length; a format vsnprintf refuses is named instead.
VKM_LOG_PRINTF(1, 0) std::string formatted(const char* fmt, va_list args) {
    va_list again;
    va_copy(again, args);
    const int length = std::vsnprintf(nullptr, 0, fmt, args);
    if (length < 0) {
        va_end(again);
        return std::string("(cannot format \"") + fmt + "\")";
    }
    std::string text(static_cast<size_t>(length) + 1, '\0');
    std::vsnprintf(text.data(), text.size(), fmt, again);
    va_end(again);
    text.pop_back();
    return text;
}

} // namespace

void stopProgram() {
#ifdef _WIN32
    __debugbreak();
#else
    abort();
#endif
}

void assertFail(
    const char* suffix,
    const char* category,
    const char* condition,
    const char* file,
    int line,
    const char* fmt,
    ...
) {
    const std::string where = std::string(file) + ":" + std::to_string(line);
    std::string message     = "VKM ASSERT Failed!\t[" + where + "] Expected: '" + condition + "'";
    if (fmt != nullptr) {
        va_list args;
        va_start(args, fmt);
        message += ", Info: " + formatted(fmt, args);
        va_end(args);
    }

    try {
        Logger::getInstance().log(Level::Error, suffix, category, "%s", message.c_str());
    } catch (const std::runtime_error&) {
        // Before Logger::init: the line goes to stderr, and the program still stops.
        std::fprintf(stderr, "%s\n", message.c_str());
        std::fflush(stderr);
    }
    stopProgram();
}

} // namespace Vkm::Log
