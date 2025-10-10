#include "l_assert.h"

#include <stdio.h>
// Linux g++ specific for abort()
#include <stdlib.h>

#include "logger.h"

void __assert(
    bool expected,
    const char* expected_str,
    const char* file,
    int line,
    const char* message
) {
    if (expected) {
        return;
    }

    LOG_ERROR("VKM ASSERT Failed!\t[%s:%d] Expected: '%s', Info: %s", file, line, expected_str, message);

#ifdef _WIN32
        __debugbreak();
#else
        abort();
#endif
}
