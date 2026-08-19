#include "l_assert.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "logger.h"

namespace Vkm::Log {

void assertFail(
    const char* expected_str,
    const char* file,
    int line,
    const char* fmt,
    ...
) {
    if (fmt == nullptr) {
        LOG_ERROR("VKM ASSERT Failed!\t[%s:%d] Expected: '%s'",
            file, line, expected_str);
    } else {
        char buf[512];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        LOG_ERROR("VKM ASSERT Failed!\t[%s:%d] Expected: '%s', Info: %s",
            file, line, expected_str, buf);
    }

#ifdef _WIN32
    __debugbreak();
#else
    abort();
#endif
}

} // namespace Vkm::Log
