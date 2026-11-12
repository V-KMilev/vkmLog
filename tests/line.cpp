// A line as the logger writes it, and parseLine reading it back.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "vkm_log/logger.h"

#include "harness/check.h"
#include "harness/scratch.h"

namespace {

using Vkm::Log::Level;
using Vkm::Log::Logger;
using Vkm::Log::parseLine;

/// The lines of @p path, without their newlines.
std::vector<std::string> linesOf(const std::filesystem::path& path) {
    std::vector<std::string> lines;
    const std::string text = Vkm::Test::readText(path);
    size_t at = 0;
    for (size_t end = text.find('\n'); end != std::string::npos; end = text.find('\n', at)) {
        lines.push_back(text.substr(at, end - at));
        at = end + 1;
    }
    return lines;
}

/// A logger writing every level to the test's own file.
std::filesystem::path openLog() {
    const std::filesystem::path path = Vkm::Test::scratch() / "test.log";
    REQUIRE(Logger::init(path, "VKM-TESTS", Level::Trace));
    return path;
}

/// @p text as parseLine reads it; the test stops when it is not a line.
Vkm::Log::Line parsed(std::string_view text) {
    const auto line = parseLine(text);
    REQUIRE(line.has_value());
    return *line;
}

/// Whether @p time has the shape "0000-00-00 00:00:00.000", each '0' a digit.
bool isStamp(std::string_view time) {
    const std::string_view shape = "0000-00-00 00:00:00.000";
    if (time.size() != shape.size()) return false;
    for (size_t i = 0; i < time.size(); ++i) {
        const bool digit = time[i] >= '0' && time[i] <= '9';
        if (shape[i] == '0' ? !digit : time[i] != shape[i]) return false;
    }
    return true;
}

/// Now, local time, as "YYYY-MM-DD HH:MM:SS.mmm": its text sorts as time does, but across the instant a
/// clock turns back.
std::string stampNow() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t clock = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &clock);
#else
    localtime_r(&clock, &local);
#endif
    const auto sinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    const int millisecond = static_cast<int>(sinceEpoch.count() % 1000);

    char text[32];
    const size_t length = std::strftime(text, sizeof(text), "%Y-%m-%d %H:%M:%S", &local);
    std::snprintf(text + length, sizeof(text) - length, ".%03d", millisecond);
    return text;
}

} // namespace

TEST("each level has its own name, and a value outside them is UNKNOWN") {
    CHECK(Vkm::Log::levelName(Level::Trace) == "TRACE");
    CHECK(Vkm::Log::levelName(Level::Verbose) == "VERBOSE");
    CHECK(Vkm::Log::levelName(Level::Debug) == "DEBUG");
    CHECK(Vkm::Log::levelName(Level::Info) == "INFO");
    CHECK(Vkm::Log::levelName(Level::Warning) == "WARNING");
    CHECK(Vkm::Log::levelName(Level::Error) == "ERROR");
    CHECK(Vkm::Log::levelName(Level::Fatal) == "FATAL");
    CHECK(Vkm::Log::levelName(Level::Count) == "UNKNOWN");
}

TEST("a line logged at each level parses back to that level") {
    const std::filesystem::path path = openLog();
    for (int i = 0; i < static_cast<int>(Level::Count); ++i) {
        Logger::getInstance().log(static_cast<Level>(i), nullptr, nullptr, "level %d", i);
    }

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == static_cast<size_t>(Level::Count));
    for (int i = 0; i < static_cast<int>(Level::Count); ++i) {
        const auto line = parseLine(lines[i]);
        REQUIRE(line.has_value());
        CHECK(line->level == static_cast<Level>(i));
        CHECK(line->message == "level " + std::to_string(i));
    }
}

TEST("a line with a category parses back as it was logged: its time, suffix, category, level and message") {
    const std::filesystem::path path = openLog();
    const std::string before = stampNow();
    Logger::getInstance().log(Level::Warning, nullptr, "AUDIO", "%d voices of %s", 3, "eight");
    const std::string after = stampNow();

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == 1u);
    const auto line = parseLine(lines[0]);
    REQUIRE(line.has_value());
    CHECK(line->suffix == "VKM-TESTS");
    CHECK(line->category == "AUDIO");
    CHECK(line->level == Level::Warning);
    CHECK(line->message == "3 voices of eight");

    // The local time of the call, to the millisecond, each digit of its own second.
    CHECK(isStamp(line->time));
    CHECK(before <= line->time);
    CHECK(line->time <= after);
}

TEST("a line logged without a category, by null or by empty, parses back with none") {
    const std::filesystem::path path = openLog();
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "by null");
    Logger::getInstance().log(Level::Info, nullptr, "", "by empty");

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == 2u);
    for (const std::string& text : lines) {
        const auto line = parseLine(text);
        REQUIRE(line.has_value());
        CHECK(line->category.empty());
        CHECK(line->level == Level::Info);
        CHECK(line->suffix == "VKM-TESTS");
    }
    CHECK(parsed(lines[0]).message == "by null");
    CHECK(parsed(lines[1]).message == "by empty");
}

TEST("a call naming its own suffix carries it, and a null or empty one carries the init suffix") {
    const std::filesystem::path path = openLog();
    Logger::getInstance().log(Level::Info, "VKM-OTHER", "NET", "named");
    Logger::getInstance().log(Level::Info, nullptr, "NET", "null");
    Logger::getInstance().log(Level::Info, "", "NET", "empty");

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == 3u);
    CHECK(parsed(lines[0]).suffix == "VKM-OTHER");
    CHECK(parsed(lines[1]).suffix == "VKM-TESTS");
    CHECK(parsed(lines[2]).suffix == "VKM-TESTS");
    CHECK(parsed(lines[0]).category == "NET");
}

TEST("a message's own brackets are kept whole, and a level it quotes is not the line's") {
    const std::filesystem::path path = openLog();
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "a [held] line %d", 7);
    Logger::getInstance().log(Level::Debug, nullptr, nullptr, "[x] in the text");
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "[ERROR] was the word");
    Logger::getInstance().log(Level::Info, nullptr, "CORE", "[FATAL] [WARNING] quoted");

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == 4u);
    CHECK(parsed(lines[0]).message == "a [held] line 7");
    CHECK(parsed(lines[1]).message == "[x] in the text");
    CHECK(parsed(lines[1]).level == Level::Debug);
    CHECK(parsed(lines[2]).level == Level::Info);
    CHECK(parsed(lines[2]).message == "[ERROR] was the word");
    CHECK(parsed(lines[2]).category.empty());
    CHECK(parsed(lines[3]).level == Level::Info);
    CHECK(parsed(lines[3]).category == "CORE");
    CHECK(parsed(lines[3]).message == "[FATAL] [WARNING] quoted");

    // As written by hand, with no category.
    const auto quoted = parseLine("[t] [S] [INFO] [ERROR] was the word");
    REQUIRE(quoted.has_value());
    CHECK(quoted->level == Level::Info);
    CHECK(quoted->message == "[ERROR] was the word");
}

TEST("an empty message parses, with the message empty") {
    const std::filesystem::path path = openLog();
    Logger::getInstance().log(Level::Info, nullptr, "CORE", "%s", "");

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == 1u);
    const auto line = parseLine(lines[0]);
    REQUIRE(line.has_value());
    CHECK(line->message.empty());
    CHECK(line->category == "CORE");
}

TEST("a message longer than the logger's first buffer arrives whole") {
    const std::filesystem::path path = openLog();
    const std::string message(3000, 'x');
    Logger::getInstance().log(Level::Info, nullptr, nullptr, "%s.", message.c_str());

    const std::vector<std::string> lines = linesOf(path);
    REQUIRE(lines.size() == 1u);
    CHECK(parsed(lines[0]).message == message + ".");
}

TEST("lines logged from several threads at once each arrive whole, none lost") {
    constexpr int THREADS = 8;
    constexpr int LINES   = 500;

    const std::filesystem::path path = openLog();
    std::vector<std::thread> threads;
    for (int thread = 0; thread < THREADS; ++thread) {
        threads.emplace_back([thread] {
            for (int line = 0; line < LINES; ++line) {
                Logger::getInstance().log(Level::Info, nullptr, "THREADS", "thread %d line %d", thread, line);
            }
        });
    }
    for (std::thread& thread : threads) thread.join();

    const std::vector<std::string> lines = linesOf(path);
    CHECK(lines.size() == static_cast<size_t>(THREADS * LINES));
    const auto whole = std::count_if(lines.begin(), lines.end(), [](const std::string& text) {
        const auto line = parseLine(text);
        return line && line->category == "THREADS" && line->message.rfind("thread ", 0) == 0;
    });
    CHECK(static_cast<size_t>(whole) == lines.size());
}

TEST("text with no level's name in its third or fourth bracketed field parses as nothing") {
    CHECK(!parseLine("plain words").has_value());
    CHECK(!parseLine("[t] [S] message").has_value());
    CHECK(!parseLine("[t] [S] [C] message").has_value());
    CHECK(!parseLine("[t] [S] [C] [X] message").has_value());
    CHECK(!parseLine("[t] [S").has_value());
    CHECK(!parseLine("").has_value());
}

TEST("a parsed line's fields view the text it was given, not a copy") {
    const std::string text = "[2026-01-02 03:04:05.006] [S] [CAT] [ERROR] the message";
    const auto line = parseLine(text);
    REQUIRE(line.has_value());
    const char* begin = text.data();
    const char* end   = text.data() + text.size();
    for (const std::string_view field : {line->time, line->suffix, line->category, line->message}) {
        CHECK(field.data() >= begin);
        CHECK(field.data() + field.size() <= end);
    }
    CHECK(line->message == "the message");
    CHECK(line->time == "2026-01-02 03:04:05.006");
}
