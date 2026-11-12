// Runs one test, named on the command line, in this process; ctest starts one process a test
// (discover.cmake), so a crash, a hang or a process-wide singleton is one test's alone.
//
//     <suite>_tests --list
//     <suite>_tests "<suite>: <what works>"
//
// VKM_TEST_SUITE names the suite. Each test's own folder is test_scratch/<suite>/<test> beside the
// suite's binary, which it asks the system for as it runs (executableDir, Linux and Windows), so the
// folder moves with the binary however the suite is started. With VKM_TEST_LOG, each test logs at
// Info and up into test.log in its folder (logText).

#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <map>
#include <new>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#if defined(_WIN32)
    #include <malloc.h>
#else
    #include <sys/resource.h>
    #include <sys/wait.h>
    #include <unistd.h>
#endif

// A sanitizer brings an operator new and delete of its own, to watch every block: the counting
// replacements stay out of its way, and allocations() skips.
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
    #define VKM_TEST_SANITIZED
#elif defined(__has_feature)
    #if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(memory_sanitizer)
        #define VKM_TEST_SANITIZED
    #endif
#endif

#if defined(VKM_TEST_LOG)
    #include "vkm_log/logger.h"
#endif

#include "harness/check.h"
#include "harness/scratch.h"

// Last, as its macros reach only what follows.
#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef NOGDI
        #define NOGDI
    #endif
    #include <windows.h>
#endif

namespace Vkm::Test {

namespace {

struct Entry {
    std::string name;
    Body        body = nullptr;
};

std::vector<Entry>& entries() {
    static std::vector<Entry> s_entries;
    return s_entries;
}

int g_failures = 0;

std::filesystem::path g_scratch;

/// @p name as a folder name: what a file system takes, nothing else.
std::string folderFor(const std::string& name) {
    std::string out;
    for (const char c : name) {
        const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (plain) out += c;
        else if (!out.empty() && out.back() != '_') out += '_';
    }
    return out.substr(0, 80);
}

/// The folder the suite's binary is in, asked of the system as it runs: a path built in would name
/// the building machine's, which a binary built for another system runs without.
std::filesystem::path executableDir() {
#if defined(_WIN32)
    wchar_t path[MAX_PATH];
    const DWORD length = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};  // failed or cut short
    return std::filesystem::path(path, path + length).parent_path();
#else
    std::error_code ec;
    return std::filesystem::read_symlink("/proc/self/exe", ec).parent_path();
#endif
}

int run(const std::string& full, const Entry& entry) {
    const std::filesystem::path home = executableDir();
    if (home.empty()) {
        std::printf("The suite's own folder cannot be found: '%s' has nowhere to run.\n", full.c_str());
        return 2;
    }
    g_scratch = home / "test_scratch" / VKM_TEST_SUITE / folderFor(entry.name);
    std::error_code ec;
    std::filesystem::remove_all(g_scratch, ec);
    if (!ec) std::filesystem::create_directories(g_scratch, ec);
    if (ec) {
        std::printf(
            "'%s' has no empty folder at %s: %s.\n",
            full.c_str(),
            g_scratch.string().c_str(),
            ec.message().c_str()
        );
        return 2;
    }

#if defined(VKM_TEST_LOG)
    Log::Logger::init(g_scratch / "test.log", "VKM-TESTS", Log::Level::Info);
#endif

    std::printf("%s\n", full.c_str());
    try {
        entry.body();
    } catch (const Stop&) {
    } catch (const Skip& skipped) {
        std::printf("SKIPPED: %s\n", skipped.reason.c_str());
        // A check that failed before the skip still fails the test.
        if (g_failures == 0) return 77;
    } catch (const std::exception& e) {
        fail("", 0, std::string("threw ") + e.what());
    }
    std::printf(g_failures == 0 ? "ok\n" : "FAILED\n");
    return g_failures == 0 ? 0 : 1;
}

#if !defined(VKM_TEST_SANITIZED)
std::atomic<uint64_t> g_allocations{0};
#endif

} // namespace

Registration::Registration(const char* name, Body body) { entries().push_back({name, body}); }

void fail(const char* file, int line, const std::string& what) {
    ++g_failures;
    const std::string name  = std::filesystem::path(file).filename().string();
    const std::string where = file[0] ? name + ":" + std::to_string(line) : std::string("test");
    std::printf("  %s: %s\n", where.c_str(), what.c_str());
}

const std::filesystem::path& scratch() { return g_scratch; }

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream(path, std::ios::binary) << text;
}

std::string readText(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

std::string logText() { return readText(g_scratch / "test.log"); }

Ended inChild(const std::function<void()>& body) {
#if defined(_WIN32)
    (void)body;
    skip("a child process here needs fork");
#else
    std::fflush(nullptr);
    const pid_t child = fork();
    if (child == 0) {
        // An abort is the expected end: no core file for it.
        const rlimit none{0, 0};
        setrlimit(RLIMIT_CORE, &none);
        try {
            body();
        } catch (...) {
            _exit(3);
        }
        std::fflush(nullptr);
        _exit(0);
    }
    int status = 0;
    if (child < 0 || waitpid(child, &status, 0) != child) return Ended::Otherwise;
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) return Ended::Returned;
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) return Ended::Stopped;
    return Ended::Otherwise;
#endif
}

uint64_t allocations() {
#if defined(VKM_TEST_SANITIZED)
    skip("a sanitizer's own operator new is not counted");
#else
    return g_allocations.load(std::memory_order_relaxed);
#endif
}

} // namespace Vkm::Test

#if !defined(VKM_TEST_SANITIZED)

namespace {

/// A block of @p size bytes, counted for allocations(); null when there is no memory for it.
void* counted(std::size_t size) {
    Vkm::Test::g_allocations.fetch_add(1, std::memory_order_relaxed);
    return std::malloc(size ? size : 1);
}

/// A block of @p size bytes on an @p align boundary, counted; null when there is no memory for it.
void* countedAligned(std::size_t size, std::align_val_t align) {
    Vkm::Test::g_allocations.fetch_add(1, std::memory_order_relaxed);
    const std::size_t alignment = static_cast<std::size_t>(align);
#if defined(_WIN32)
    return _aligned_malloc(size ? size : 1, alignment);
#else
    // aligned_alloc takes a whole number of alignments.
    return std::aligned_alloc(alignment, (size / alignment + 1) * alignment);
#endif
}

/// Returns a block countedAligned gave.
void releaseAligned(void* block) {
#if defined(_WIN32)
    _aligned_free(block);
#else
    std::free(block);
#endif
}

} // namespace

// GCC 11 and up, inlining a delete here or in main, sees std::free take a block from operator new;
// these replacements are the pair that makes it right, so the warning is off to the end of the file.
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ >= 11
    #pragma GCC diagnostic ignored "-Wmismatched-new-delete"
#endif

// Every form, counted for allocations(), the library's own calls too where the platform resolves them
// here: not inside a Windows DLL, whose calls stay with its own runtime.
void* operator new(std::size_t size) {
    if (void* block = counted(size)) return block;
    throw std::bad_alloc();
}
void* operator new(std::size_t size, std::align_val_t align) {
    if (void* block = countedAligned(size, align)) return block;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return operator new(size); }
void* operator new[](std::size_t size, std::align_val_t align) { return operator new(size, align); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return counted(size); }
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept { return counted(size); }
void* operator new(std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return countedAligned(size, align);
}
void* operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t&) noexcept {
    return countedAligned(size, align);
}
void operator delete(void* block) noexcept { std::free(block); }
void operator delete[](void* block) noexcept { std::free(block); }
void operator delete(void* block, std::size_t) noexcept { std::free(block); }
void operator delete[](void* block, std::size_t) noexcept { std::free(block); }
void operator delete(void* block, const std::nothrow_t&) noexcept { std::free(block); }
void operator delete[](void* block, const std::nothrow_t&) noexcept { std::free(block); }
void operator delete(void* block, std::align_val_t) noexcept { releaseAligned(block); }
void operator delete[](void* block, std::align_val_t) noexcept { releaseAligned(block); }
void operator delete(void* block, std::size_t, std::align_val_t) noexcept { releaseAligned(block); }
void operator delete[](void* block, std::size_t, std::align_val_t) noexcept { releaseAligned(block); }
void operator delete(void* block, std::align_val_t, const std::nothrow_t&) noexcept { releaseAligned(block); }
void operator delete[](void* block, std::align_val_t, const std::nothrow_t&) noexcept {
    releaseAligned(block);
}

#endif

int main(int argc, char** argv) {
    using namespace Vkm::Test;

    // Unbuffered, as ctest reads it through a pipe: a crash keeps what was printed before it.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    std::map<std::string, const Entry*> byName;
    for (const Entry& entry : entries()) {
        const std::string full = std::string(VKM_TEST_SUITE) + ": " + entry.name;
        // ctest's lists split on ';'.
        if (full.find(';') != std::string::npos) {
            std::printf("A test's name holds a ';': '%s'.\n", full.c_str());
            return 2;
        }
        if (!byName.emplace(full, &entry).second) {
            std::printf("Two tests are called '%s'.\n", full.c_str());
            return 2;
        }
    }

    if (argc == 2 && std::strcmp(argv[1], "--list") == 0) {
        for (const auto& [full, entry] : byName) std::printf("%s\n", full.c_str());
        return 0;
    }
    if (argc != 2) {
        std::printf("Usage: %s --list | \"%s: <test>\"\n", argv[0], VKM_TEST_SUITE);
        return 2;
    }
    const auto found = byName.find(argv[1]);
    if (found == byName.end()) {
        std::printf("No test called '%s'; --list names them.\n", argv[1]);
        return 2;
    }

    const int status = run(found->first, *found->second);
    if (status == 0) {
        std::error_code ec;
        std::filesystem::remove_all(g_scratch, ec);
    }
    std::fflush(stdout);
    return status;
}
