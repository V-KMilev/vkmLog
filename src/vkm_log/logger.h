#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

// A printf-style function's format, checked against its arguments where the compiler can: the
// 1-based places of the format and of its first argument (a member function's `this` is 1).
#if defined(__GNUC__) || defined(__clang__)
    #define VKM_LOG_PRINTF(formatAt, firstAt) __attribute__((format(printf, formatAt, firstAt)))
#else
    #define VKM_LOG_PRINTF(formatAt, firstAt)
#endif

namespace Vkm::Log {

/**
 * @brief Log levels for the Logger.
 */
enum class Level {
    Trace,
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Count  ///< Sentinel; keep last.
};

/**
 * @brief The name a line carries for @p level, as the writer puts it between brackets.
 *
 * @param level Any value; one outside Trace to Fatal, such as Level::Count, has no level's name.
 * @return "TRACE", "VERBOSE", "DEBUG", "INFO", "WARNING", "ERROR" or "FATAL"; "UNKNOWN" outside them.
 */
std::string_view levelName(Level level);

/// One line as the logger writes it, each field a view into the text it was read from.
struct Line {
    std::string_view time;                 ///< "YYYY-MM-DD HH:MM:SS.mmm", local time.
    std::string_view suffix;               ///< The init suffix, or the one the call named.
    std::string_view category;             ///< Empty when the line has none.
    Level            level = Level::Info;
    std::string_view message;              ///< All after the level, its own brackets kept.
};

/**
 * @brief Split @p text as the logger writes a line: "[time] [suffix] [category] [LEVEL] message".
 *
 * The level is the third bracketed field, or the fourth when a category comes first, so a message
 * opening with brackets of its own keeps them.
 *
 * @param text One line, without its newline.
 * @return Its fields, viewing @p text; nothing when neither its third nor its fourth leading bracketed
 *         field is a level's name.
 */
std::optional<Line> parseLine(std::string_view text);

/**
 * @brief Thread-safe singleton logger class for logging messages to file and console.
 *
 * Usage:
 *   - Initialize with Logger::init(path, suffix, level)
 *   - Use LOG_* macros for logging
 */
class Logger {
    public:
        Logger() = delete;
        ~Logger() = default;

        Logger(const Logger& other) = delete;
        Logger& operator=(const Logger& other) = delete;

        Logger(Logger && other) = delete;
        Logger& operator=(Logger && other) = delete;

    public:
        /**
         * @brief Initializes the logger singleton, writing to the console and to @p path.
         *
         * Lines reach the console whether or not the file opened. After false, init may be called
         * again with another file, before any other thread logs.
         *
         * @param path   The log file, appended to; its folder is made when missing. (default: log.log)
         * @param suffix Suffix added to the log message (default: ROOT)
         * @param level  Minimum log level to output. (default: Info)
         * @return Whether the file opened; false too when a logger with a file already exists.
         */
        static bool init(
            const std::filesystem::path& path = "log.log",
            const std::string& suffix = "ROOT",
            Level level = Level::Info
        );

        /**
         * @brief The logger init made, for every LOG_* call.
         *
         * @return The one logger.
         * @throws std::runtime_error when init was never called.
         */
        static Logger& getInstance();

        /**
         * @brief Logs a formatted message with the given log level.
         * @param level Log level for the message.
         * @param suffix Optional suffix override for this line. Pass nullptr or
         *               "" to fall back to the Logger's init suffix.
         * @param category Optional subsystem tag printed between the suffix
         *                 and the level (e.g. "CORE", "RENDER", "SHADER").
         *                 Pass nullptr or "" to omit the tag entirely.
         * @param format printf-style format string.
         * @param ... Arguments for the format string.
         */
        void log(Level level, const char* suffix, const char* category, const char* format, ...)
            VKM_LOG_PRINTF(5, 6);

    public:
        /// The file lines are appended to; empty when it did not open.
        const std::filesystem::path& path() const { return m_path; }

        /// The file's size when this run opened it: where this run's lines begin.
        std::uintmax_t runStart() const { return m_runStart; }

    private:
        /**
         * @brief Private constructor. Only accessible from getInstance/init.
         * @param path The log file; the logger has none when it will not open.
         * @param suffix Suffix added to the log message
         * @param level Minimum log level to output.
         */
        Logger(
            const std::filesystem::path& path,
            const std::string& suffix,
            Level level
        );

    private:
        std::filesystem::path m_path;                ///< Empty when the file did not open.
        std::uintmax_t        m_runStart = 0;
        std::string           m_suffix;              ///< A line's suffix when its call names none.
        Level                 m_level = Level::Info; ///< The least a line must be to be written.

        std::ofstream m_file;
        std::mutex    m_mutex;  ///< One line at a time, to the console and the file.

        static std::unique_ptr<Logger> s_instance;
};

} // namespace Vkm::Log

// The suffix a library's lines carry in place of the init suffix, so they say whose they are.
// A library defines it once for its whole target (target_compile_definitions), never per file.
// Left undefined, a line carries the init suffix.
#ifndef VKM_LOG_SUFFIX
    #define VKM_LOG_SUFFIX nullptr
#endif

// The category every LOG_* call in a translation unit is tagged with, defined before the include:
//   #define VKM_LOG_CATEGORY "RENDER"
//   #include "vkm_log/logger.h"
// Left undefined, a line has no category bracket.
#ifndef VKM_LOG_CATEGORY
    #define VKM_LOG_CATEGORY nullptr
#endif

// Macros for easy logging. Suffix + category come from VKM_LOG_SUFFIX and
// VKM_LOG_CATEGORY at the call site; use LOG_*_C(category, ...) variants to
// override the category for a single call.
#define LOG(level, format, ...) \
    Vkm::Log::Logger::getInstance().log(level, VKM_LOG_SUFFIX, VKM_LOG_CATEGORY, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...)   LOG(Vkm::Log::Level::Trace, format, ##__VA_ARGS__)
#define LOG_VERBOSE(format, ...) LOG(Vkm::Log::Level::Verbose, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)   LOG(Vkm::Log::Level::Debug, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)    LOG(Vkm::Log::Level::Info, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) LOG(Vkm::Log::Level::Warning, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)   LOG(Vkm::Log::Level::Error, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)   LOG(Vkm::Log::Level::Fatal, format, ##__VA_ARGS__)

// Explicit-category variants. Use for inline log calls in headers, or when a
// single line needs to override the file's default category. Suffix still
// comes from the call site's VKM_LOG_SUFFIX.
#define LOG_C(level, category, format, ...) \
    Vkm::Log::Logger::getInstance().log(level, VKM_LOG_SUFFIX, category, format, ##__VA_ARGS__)
#define LOG_TRACE_C(category, format, ...)   LOG_C(Vkm::Log::Level::Trace, category, format, ##__VA_ARGS__)
#define LOG_VERBOSE_C(category, format, ...) LOG_C(Vkm::Log::Level::Verbose, category, format, ##__VA_ARGS__)
#define LOG_DEBUG_C(category, format, ...)   LOG_C(Vkm::Log::Level::Debug, category, format, ##__VA_ARGS__)
#define LOG_INFO_C(category, format, ...)    LOG_C(Vkm::Log::Level::Info, category, format, ##__VA_ARGS__)
#define LOG_WARNING_C(category, format, ...) LOG_C(Vkm::Log::Level::Warning, category, format, ##__VA_ARGS__)
#define LOG_ERROR_C(category, format, ...)   LOG_C(Vkm::Log::Level::Error, category, format, ##__VA_ARGS__)
#define LOG_FATAL_C(category, format, ...)   LOG_C(Vkm::Log::Level::Fatal, category, format, ##__VA_ARGS__)
