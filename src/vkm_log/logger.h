#pragma once

#include <fstream>
#include <string>
#include <mutex>
#include <memory>

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
    Fatal
};

/**
 * @brief Thread-safe singleton logger class for logging messages to file and console.
 *
 * Usage:
 *   - Initialize once with Logger::init(filename, suffix,level)
 *   - Use LOG_* macros for logging
 */
class Logger {
    public:
        Logger() = delete;
        ~Logger();

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

    public:
        /**
         * @brief Initializes the logger singleton.
         * @param filename Path to the log file. (default: log.log)
         * @param suffix Suffix aded to the log message (default: ROOT)
         * @param level Minimum log level to output. (default: INFO)
         * @return true if initialization succeeded, false if already initialized.
         */
        static bool init(
            const std::string& filename = "log.log",
            const std::string& suffix = "ROOT",
            Level level = Level::Info
        );

        /**
         * @brief Gets the singleton instance of the logger.
         * @return Reference to the Logger instance.
         */
        static Logger& getInstance();

        /**
         * @brief Logs a formatted message with the given log level.
         * @param level Log level for the message.
         * @param suffix Optional suffix override for this line (e.g. "VKM-GL"
         *               for vkmGL-library calls). Pass nullptr or "" to fall
         *               back to the Logger's init suffix.
         * @param category Optional subsystem tag printed between the suffix
         *                 and the level (e.g. "CORE", "RENDER", "SHADER").
         *                 Pass nullptr or "" to omit the tag entirely.
         * @param format printf-style format string.
         * @param ... Arguments for the format string.
         */
        void log(Level level, const char* suffix, const char* category, const char* format, ...);

    private:
        /**
         * @brief Private constructor. Only accessible from getInstance/init.
         * @param filename Path to the log file.
         * @param suffix Suffix aded to the log message
         * @param level Minimum log level to output.
         */
        Logger(
            const std::string& filename,
            const std::string& suffix,
            Level level
        );

        /**
         * @brief Converts a log level to its string representation.
         * @param level Log level.
         * @return String representation of the log level.
         */
        std::string levelToString(Level level);

        /**
         * @brief Gets the current timestamp as a string.
         * @return Timestamp string in "YYYY-MM-DD HH:MM:SS" format.
         */
        std::string getTimestamp();

    private:
        std::string m_filename;
        std::string m_suffix;
        Level m_level;

        std::ofstream m_file;
        std::mutex m_mutex;

        static bool s_isInitialized;
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
