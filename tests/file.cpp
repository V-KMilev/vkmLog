// Logger::init and the file it writes: where the lines go when it opens, and when it will not.

#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>

#if defined(_WIN32)
    #include <io.h>
#else
    #include <unistd.h>
#endif

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

TEST("init on a path that cannot be made answers false, and its lines reach the console alone") {
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
