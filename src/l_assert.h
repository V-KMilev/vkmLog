#pragma once

namespace Vkm::Log {

/**
 * @brief Assertion failure handler. Logs and aborts; never returns.
 *
 * Called by VKM_ASSERT when its condition is false. Accepts an optional
 * printf-style format string + arguments so callers can include diagnostic
 * values inline:
 *
 *     VKM_ASSERT(ptr != nullptr);                       // no message
 *     VKM_ASSERT(ok, "load failed");                    // static message
 *     VKM_ASSERT(idx < n, "slot %u out of %zu", idx, n);// printf-style
 *
 * @param expected_str String form of the failed condition (from the macro).
 * @param file Source file where the assertion failed.
 * @param line Line in @p file.
 * @param fmt  Optional printf-style format. nullptr means no message.
 * @param ...  printf-style varargs for @p fmt.
 */
void assertFail(
    const char* expected_str,
    const char* file,
    int line,
    const char* fmt = nullptr,
    ...
);

} // namespace Vkm::Log

/**
 * @brief Assertion macro with logging, source location, and optional printf-
 *        style message.
 *
 * Evaluates 'Expected'; on false, calls Vkm::Log::assertFail, which logs and
 * aborts. The macro short-circuits cheaply so a passing assert costs only the
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
// Release build: disable assertion checking for performance.
 #define VKM_ASSERT(Expected, ...) ((void)0)
#else
// Debug build: evaluate the condition once; on failure pass everything to the
// handler. The ##__VA_ARGS__ GCC/Clang extension eats the leading comma when no
// message is supplied, so VKM_ASSERT(cond) compiles cleanly.
#define VKM_ASSERT(Expected, ...)                                               \
    do {                                                                        \
        if (!(Expected))                                                        \
            Vkm::Log::assertFail(#Expected, __FILE__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif
