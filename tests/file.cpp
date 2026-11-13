// Logger::init and the file it writes: where the lines go when it opens, and when it will not.

#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <functional>
#include <string>

#if defined(_WIN32)
    #include <io.h>
#else
    #include <unistd.h>
#endif

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

#include "harness/check.h"
#include "harness/scratch.h"

namespace {

namespace fs = std::filesystem;

using Vkm::Log::Level;
using Vkm::Log::Logger;
using Vkm::Test::readText;
using Vkm::Test::scratch;

/// What the process writes to its console while @p body runs.
std::string consoleOf(const std::function<void()>& body) {
    const fs::path captured = scratch() / "console.txt";
    std::fflush(stdout);
#if defined(_WIN32)
    const int saved = _dup(_fileno(stdout));
    FILE* file = std::fopen(captured.string().c_str(), "wb");
    _dup2(_fileno(file), _fileno(stdout));
    body();
    std::fflush(stdout);
    _dup2(saved, _fileno(stdout));
    _close(saved);
#else
    const int saved = dup(fileno(stdout));
    FILE* file = std::fopen(captured.string().c_str(), "wb");
    dup2(fileno(file), fileno(stdout));
    body();
    std::fflush(stdout);
    dup2(saved, fileno(stdout));
    close(saved);
#endif
    std::fclose(file);
    return readText(captured);
}

} // namespace

TEST("init makes the log's missing folders, answers true, and the new file's run starts at its first byte") {
    const fs::path path = scratch() / "a" / "b" / "run.log";
    CHECK(Logger::init(path, "VKM-TESTS", Level::Info));
    CHECK(Logger::getInstance().path() == path);
    CHECK(Logger::getInstance().runStart() == 0u);

    Logger::getInstance().log(Level::Info, nullptr, nullptr, "first");
    CHECK(readText(path).find("first") != std::string::npos);
}

TEST("init on a path that cannot be made answers false, says so on the console, and logs there alone") {
    // A folder under a regular file cannot be made.
    Vkm::Test::writeText(scratch() / "file", "x");
    const fs::path path = scratch() / "file" / "sub" / "run.log";

    bool opened = true;
    const std::string console = consoleOf([&] {
        opened = Logger::init(path, "VKM-TESTS", Level::Info);
        Logger::getInstance().log(Level::Warning, nullptr, nullptr, "still heard");
    });
    CHECK(!opened);
    CHECK(Logger::getInstance().path().empty());
    CHECK(console.find("cannot open " + path.string() + " (") != std::string::npos);
    CHECK(console.find("[WARNING] still heard") != std::string::npos);
    CHECK(!fs::exists(path));
}

TEST("after init fails, init with a writable path answers true and its file takes the next line") {
    Vkm::Test::writeText(scratch() / "file", "x");
    CHECK(!Logger::init(scratch() / "file" / "run.log", "VKM-TESTS", Level::Info));

    const fs::path path = scratch() / "run.log";
    CHECK(Logger::init(path, "VKM-TESTS", Level::Info));
    CHECK(Logger::getInstance().path() == path);
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "the next line");
    CHECK(readText(path).find("[INFO] the next line") != std::string::npos);
}

TEST("after init succeeds, a second init answers false and the first file keeps taking lines") {
    const fs::path first  = scratch() / "first.log";
    const fs::path second = scratch() / "second.log";
    REQUIRE(Logger::init(first, "VKM-TESTS", Level::Info));
    CHECK(!Logger::init(second, "OTHER", Level::Trace));

    CHECK(Logger::getInstance().path() == first);
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "kept");
    Logger::getInstance().log(Level::Debug, nullptr, nullptr, "below the first init's level");
    CHECK(readText(first).find("[VKM-TESTS] [INFO] kept") != std::string::npos);
    CHECK(readText(first).find("below") == std::string::npos);
    CHECK(!fs::exists(second));
}

TEST("runStart is the file's size before this run, so a reader starting there reads this run's lines alone") {
    const fs::path path = scratch() / "run.log";
    const std::string earlier = "[2026-01-01 00:00:00.000] [OLD] [INFO] an earlier run\n";
    Vkm::Test::writeText(path, earlier);

    REQUIRE(Logger::init(path, "VKM-TESTS", Level::Info));
    CHECK(Logger::getInstance().runStart() == earlier.size());
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "this run");

    const std::string text = readText(path);
    REQUIRE(text.size() > earlier.size());
    CHECK(text.substr(0, earlier.size()) == earlier);
    const std::string thisRun = text.substr(Logger::getInstance().runStart());
    CHECK(thisRun.find("earlier") == std::string::npos);
    CHECK(thisRun.find("[VKM-TESTS] [INFO] this run") == 26u);
}

TEST("a file that will not take a line is said on the console once, and every line still goes there") {
#if !defined(__linux__)
    Vkm::Test::skip("no /dev/full, a file every write to fails");
#else
    REQUIRE(Logger::init("/dev/full", "VKM-TESTS", Level::Info));
    const std::string console = consoleOf([] {
        Logger::getInstance().log(Level::Info, nullptr, nullptr, "first");
        Logger::getInstance().log(Level::Info, nullptr, nullptr, "second");
    });
    CHECK(console.find("[INFO] first") != std::string::npos);
    CHECK(console.find("[INFO] second") != std::string::npos);
    const size_t said = console.find("cannot write to /dev/full");
    CHECK(said != std::string::npos);
    CHECK(console.find("cannot write", said + 1) == std::string::npos);
    // The logger keeps its file, so other threads' logger is not replaced under them.
    CHECK(!Logger::init(scratch() / "other.log", "VKM-TESTS", Level::Info));
#endif
}

TEST("the logger's own console lines are flushed as a logged line is, so a stop right after keeps them") {
    Vkm::Test::writeText(scratch() / "file", "x");
    const fs::path captured = scratch() / "console.txt";
    const Vkm::Test::Ended ended = Vkm::Test::inChild([&] {
        // A file, as a pipe would be: fully buffered.
        if (!std::freopen(captured.string().c_str(), "w", stdout)) return;
        Logger::init(scratch() / "file" / "run.log", "VKM-TESTS", Level::Info);
        Vkm::Log::stopProgram();
    });
    CHECK(ended == Vkm::Test::Ended::Stopped);
    CHECK(readText(captured).find("cannot open") != std::string::npos);
}

TEST("a format the C library cannot apply drops its line and says so, naming the format") {
    // In the C locale, a wide character outside ASCII has no multibyte form.
    const wint_t wide = 0x1234;
    if (std::snprintf(nullptr, 0, "%lc", wide) >= 0) {
        Vkm::Test::skip("this C library formats U+1234 in the C locale");
    }

    const fs::path path = scratch() / "run.log";
    REQUIRE(Logger::init(path, "VKM-TESTS", Level::Info));
    const std::string console = consoleOf([&] {
        Logger::getInstance().log(Level::Info, nullptr, nullptr, "wide %lc here", wide);
    });
    CHECK(console.find("cannot format \"wide %lc here\"") != std::string::npos);
    CHECK(readText(path).find("wide") == std::string::npos);
}

TEST("a line below the init level is written nowhere, and one at it everywhere") {
    const fs::path path = scratch() / "run.log";
    REQUIRE(Logger::init(path, "VKM-TESTS", Level::Warning));

    const std::string console = consoleOf([] {
        Logger::getInstance().log(Level::Info, nullptr, nullptr, "too quiet");
        Logger::getInstance().log(Level::Trace, nullptr, nullptr, "far too quiet");
        Logger::getInstance().log(Level::Warning, nullptr, nullptr, "loud enough");
    });
    CHECK(console.find("quiet") == std::string::npos);
    CHECK(readText(path).find("quiet") == std::string::npos);
    CHECK(console.find("[WARNING] loud enough") != std::string::npos);
    CHECK(readText(path).find("[WARNING] loud enough") != std::string::npos);
}
