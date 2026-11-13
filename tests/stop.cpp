// How a failed check stops the program: stopProgram, and VKM_ASSERT where NDEBUG is not defined.

// VKM_ASSERT as a build without NDEBUG sees it, whatever this suite's build type defines.
#undef NDEBUG

// Whose lines these are, as a library's target and file say it.
#define VKM_LOG_SUFFIX   "VKM-CALLER"
#define VKM_LOG_CATEGORY "CHECKS"

#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <string>
#include <string_view>

#include "vkm_log/assert.h"
#include "vkm_log/logger.h"

#include "harness/check.h"
#include "harness/scratch.h"

using Vkm::Test::Ended;
using Vkm::Test::inChild;

TEST("stopProgram ends the process abnormally") {
    CHECK(inChild([] { Vkm::Log::stopProgram(); }) == Ended::Stopped);
    CHECK(inChild([] {}) == Ended::Returned);
}

TEST("a failing VKM_ASSERT logs one ERROR line with its file, line, expression and message, then stops") {
    const std::filesystem::path path = Vkm::Test::scratch() / "test.log";
    const int line = __LINE__ + 3;
    const Ended ended = inChild([&] {
        Vkm::Log::Logger::init(path, "VKM-TESTS", Vkm::Log::Level::Info);
        VKM_ASSERT(1 + 1 == 3, "fmt %d", 1);
        Vkm::Log::Logger::getInstance().log(Vkm::Log::Level::Info, nullptr, nullptr, "went on");
    });
    CHECK(ended == Ended::Stopped);

    const std::string text = Vkm::Test::readText(path);
    REQUIRE(!text.empty());
    CHECK(text.find('\n') == text.size() - 1);
    const auto logged = Vkm::Log::parseLine(std::string_view(text).substr(0, text.size() - 1));
    REQUIRE(logged.has_value());
    CHECK(logged->level == Vkm::Log::Level::Error);
    const std::string message(logged->message);
    CHECK(message.find(std::string(__FILE__) + ":" + std::to_string(line)) != std::string::npos);
    CHECK(message.find("1 + 1 == 3") != std::string::npos);
    CHECK(message.find("fmt 1") != std::string::npos);
}

TEST("a failing VKM_ASSERT's line carries the suffix and category of the code that asserted") {
    const std::filesystem::path path = Vkm::Test::scratch() / "test.log";
    const Ended ended = inChild([&] {
        Vkm::Log::Logger::init(path, "VKM-TESTS", Vkm::Log::Level::Info);
        VKM_ASSERT(false, "whose");
    });
    CHECK(ended == Ended::Stopped);

    const std::string text = Vkm::Test::readText(path);
    REQUIRE(!text.empty());
    const auto logged = Vkm::Log::parseLine(std::string_view(text).substr(0, text.size() - 1));
    REQUIRE(logged.has_value());
    CHECK(logged->suffix == "VKM-CALLER");
    CHECK(logged->category == "CHECKS");
}

TEST("a failing VKM_ASSERT's message arrives whole, however long") {
    const std::filesystem::path path = Vkm::Test::scratch() / "test.log";
    const std::string info(2000, 'x');
    const Ended ended = inChild([&] {
        Vkm::Log::Logger::init(path, "VKM-TESTS", Vkm::Log::Level::Info);
        VKM_ASSERT(false, "%s.", info.c_str());
    });
    CHECK(ended == Ended::Stopped);
    CHECK(Vkm::Test::readText(path).find(info + ".") != std::string::npos);
}

TEST("a failing VKM_ASSERT whose message cannot be formatted names its format") {
    // In the C locale, a wide character outside ASCII has no multibyte form.
    const wint_t wide = 0x1234;
    if (std::snprintf(nullptr, 0, "%lc", wide) >= 0) {
        Vkm::Test::skip("this C library formats U+1234 in the C locale");
    }

    const std::filesystem::path path = Vkm::Test::scratch() / "test.log";
    const Ended ended = inChild([&] {
        Vkm::Log::Logger::init(path, "VKM-TESTS", Vkm::Log::Level::Info);
        VKM_ASSERT(false, "wide %lc", wide);
    });
    CHECK(ended == Ended::Stopped);
    CHECK(Vkm::Test::readText(path).find("cannot format \"wide %lc\"") != std::string::npos);
}

TEST("a failing VKM_ASSERT before Logger::init still stops, its line on stderr") {
    const std::filesystem::path captured = Vkm::Test::scratch() / "stderr.txt";
    const Ended ended = inChild([&] {
        if (!std::freopen(captured.string().c_str(), "w", stderr)) return;
        try {
            VKM_ASSERT(1 + 1 == 3, "before init");
        } catch (...) {
        }
    });
    CHECK(ended == Ended::Stopped);
    const std::string text = Vkm::Test::readText(captured);
    CHECK(text.find("VKM ASSERT Failed!") != std::string::npos);
    CHECK(text.find("1 + 1 == 3") != std::string::npos);
    CHECK(text.find("before init") != std::string::npos);
}

TEST("a passing VKM_ASSERT evaluates its condition once and goes on") {
    int evaluated = 0;
    VKM_ASSERT(++evaluated == 1, "never said");
    VKM_ASSERT(evaluated == 1);
    CHECK(evaluated == 1);
}
