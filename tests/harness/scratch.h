#pragma once

// What every test has: a directory of its own, a way to say it cannot run here, a child process to
// stop in, and a count of allocations.

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace Vkm::Test {

/**
 * @brief Thrown by skip(): the test cannot run on this machine, which ctest shows as skipped (exit 77)
 *        unless a check failed before it.
 */
struct Skip {
    std::string reason;
};

/// Stop the test as skipped, saying why: a missing device, never a failing check; one that failed
/// before it still fails the test.
[[noreturn]] inline void skip(const std::string& reason) { throw Skip{reason}; }

/// This test's own directory under the build tree, empty when it starts; kept after a failure.
const std::filesystem::path& scratch();

/**
 * @brief Write @p text to @p path, making its folders.
 *
 * @param path Where.
 * @param text What, as bytes.
 */
void writeText(const std::filesystem::path& path, const std::string& text);

/// The bytes of @p path; empty when it will not open.
std::string readText(const std::filesystem::path& path);

/// Everything logged so far in this test, where the suite's harness opens a log (VKM_TEST_LOG).
std::string logText();

/**
 * @brief How a child process ended.
 */
enum class Ended {
    Returned,   ///< Its body returned.
    Stopped,    ///< As Vkm::Log::stopProgram stops a process: abort.
    Otherwise,  ///< Any other way: a throw, another signal, an exit of its own.
};

/**
 * @brief Run @p body in a child process of its own and wait for it.
 *
 * A check inside @p body counts in the child alone; the test reads what it left behind (its exit,
 * a file it wrote). Skips the test where there is no fork.
 *
 * @param body What the child runs.
 * @return How the child ended.
 */
Ended inChild(const std::function<void()>& body);

/// How many times the process has called operator new so far, in any form, on any thread: a hot path
/// that allocates nothing leaves it where it was. On Windows a DLL's own calls are not counted, and
/// under a sanitizer, which brings its own operator new, it skips the test.
uint64_t allocations();

} // namespace Vkm::Test
