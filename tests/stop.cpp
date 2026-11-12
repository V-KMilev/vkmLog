// How a failed check stops the program: stopProgram, and VKM_ASSERT where NDEBUG is not defined.

// VKM_ASSERT as a build without NDEBUG sees it, whatever this suite's build type defines.
#undef NDEBUG

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

TEST("a passing VKM_ASSERT evaluates its condition once and goes on") {
    int evaluated = 0;
    VKM_ASSERT(++evaluated == 1, "never said");
    VKM_ASSERT(evaluated == 1);
    CHECK(evaluated == 1);
}
