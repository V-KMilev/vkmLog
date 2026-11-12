#pragma once

#include "vkm_log/logger.h"

namespace Vkm::Log {

/**
 * @brief Stop the program where a check failed: break into the debugger on Windows, abort elsewhere.
 *
 * A caller logs why first. Under a debugger on Windows, continuing the run returns from it.
 */
void stopProgram();

/**
 * @brief Assertion failure handler. Logs, then stops the program (stopProgram).
 *
 * Called by VKM_ASSERT when its condition is false. Accepts an optional
 * printf-style format string + arguments so callers can include diagnostic
 * values inline:
 *
 *     VKM_ASSERT(ptr != nullptr);                       // no message
 *     VKM_ASSERT(ok, "load failed");                    // static message
 *     VKM_ASSERT(idx < n, "slot %u out of %zu", idx, n);// printf-style
 *
 * @param suffix    The asserting code's VKM_LOG_SUFFIX, so the line says whose it is.
 * @param category  The asserting code's VKM_LOG_CATEGORY.
 * @param condition String form of the failed condition (from the macro).
 * @param file      Source file where the assertion failed.
 * @param line      Line in @p file.
 * @param fmt       Optional printf-style format. nullptr means no message.
 * @param ...       printf-style varargs for @p fmt.
 */
void assertFail(
    const char* suffix,
    const char* category,
    const char* condition,
    const char* file,
    int line,
    const char* fmt = nullptr,
    ...
) VKM_LOG_PRINTF(6, 7);

} // namespace Vkm::Log

/**
 * @brief Assertion macro with logging, source location, and optional printf-
 *        style message.
 *
 * Evaluates 'Expected'; on false, calls Vkm::Log::assertFail, which logs and
 * stops the program. The macro short-circuits cheaply so a passing assert costs only the
 * condition evaluation.
 *
 * Usage:
 *   VKM_ASSERT(ptr != nullptr);
 *   VKM_ASSERT(ptr != nullptr, "Pointer should not be null");
 *   VKM_ASSERT(idx < count, "out-of-bounds: %u >= %zu", idx, count);
 *
 * @param Expected Expression to check.
 * @param ...      Optional printf-style format string + arguments.
 */
#ifdef NDEBUG
// With NDEBUG: compiled out, the condition unevaluated.
#define VKM_ASSERT(Expected, ...) ((void)0)
#else
// Without it: evaluate the condition once; on failure pass everything to the
// handler. The ##__VA_ARGS__ GCC/Clang extension eats the leading comma when no
// message is supplied, so VKM_ASSERT(cond) compiles cleanly.
#define VKM_ASSERT(Expected, ...)                                                                   \
    do {                                                                                            \
        if (!(Expected))                                                                            \
            Vkm::Log::assertFail(                                                                   \
                VKM_LOG_SUFFIX, VKM_LOG_CATEGORY, #Expected, __FILE__, __LINE__, ##__VA_ARGS__      \
            );                                                                                      \
    } while (0)
#endif
