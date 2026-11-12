#include "vkm_log/assert.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

namespace Vkm::Log {

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
    Logger& logger = Logger::getInstance();
    if (fmt == nullptr) {
        logger.log(
            Level::Error,
            suffix,
            category,
            "VKM ASSERT Failed!\t[%s:%d] Expected: '%s'",
            file,
            line,
            condition
        );
    } else {
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        logger.log(
            Level::Error,
            suffix,
            category,
            "VKM ASSERT Failed!\t[%s:%d] Expected: '%s', Info: %s",
            file,
            line,
            condition,
            buf
        );
    }
    stopProgram();
}

} // namespace Vkm::Log
