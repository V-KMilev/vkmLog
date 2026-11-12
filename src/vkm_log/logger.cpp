#include "vkm_log/logger.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cstdarg>
#include <vector>

namespace Vkm::Log {

// Static member initialization
std::unique_ptr<Logger> Logger::s_instance = nullptr;

Logger::Logger(
    const std::filesystem::path& path,
    const std::string& suffix,
    Level level
)
    : m_suffix(suffix)
    , m_level(level)
{
    // A folder that cannot be made shows as a file that will not open.
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    m_file.open(path, std::ios::app);
    if (!m_file.is_open()) return;

    m_path     = path;
    m_runStart = std::filesystem::file_size(path, ec);
    if (ec) m_runStart = 0;
}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

bool Logger::init(const std::filesystem::path& path, const std::string& suffix, Level level) {
    if (s_instance && s_instance->m_file.is_open()) {
        printf("WARNING: Logger already writes to a file; Logger::init() is refused.\n");
        return false;
    }
    s_instance = std::unique_ptr<Logger>(new Logger(path, suffix, level));
    return s_instance->m_file.is_open();
}

Logger& Logger::getInstance() {
    if (!s_instance) {
        printf("ERROR: Logger not initialized! Call Logger::init() before using.\n");
        throw std::runtime_error("Logger not initialized! Call Logger::init() before using.");
    }
    return *s_instance;
}

void Logger::log(Level level, const char* suffix, const char* category, const char* format, ...) {
    if (level < m_level) {
        return;
    }

    va_list args;
    va_start(args, format);

    std::vector<char> buffer(1024);
    int len = std::vsnprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);

    if (len < 0) {
        printf("Logger formatting error.\n");
        return;
    }

    if (static_cast<size_t>(len) >= buffer.size()) {
        buffer.resize(len + 1);
        va_start(args, format);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);
        va_end(args);
    }

    std::string formattedMsg(buffer.data());

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::ostringstream logStream;
        // Suffix is per-call when non-null/non-empty (lets a library set its
        // own identity, e.g. "VKM-GL") and falls back to the Logger's init
        // suffix otherwise.
        const char* eff = (suffix && *suffix) ? suffix : m_suffix.c_str();
        logStream << "[" << getTimestamp() << "] [" << eff << "]";
        if (category && *category) {
            logStream << " [" << category << "]";
        }
        logStream << " [" << levelToString(level) << "] " << formattedMsg << "\n";

        // Output to console, flushed. stdout is line-buffered only when it is a
        // terminal; piped to a log collector - journalctl, docker logs - it is
        // fully buffered, and a long-running headless process shows nothing at
        // all until four kilobytes have accumulated or it exits. A log nobody
        // can read while it matters is not a log.
        printf("%s", logStream.str().c_str());
        fflush(stdout);

        // Output to log file
        if (m_file.is_open()) {
            m_file << logStream.str();
            m_file.flush();
        }
    }
}

std::string Logger::levelToString(Level level) {
    switch (level) {
        case Level::Trace:   return "TRACE";
        case Level::Verbose: return "VERBOSE";
        case Level::Debug:   return "DEBUG";
        case Level::Info:    return "INFO";
        case Level::Warning: return "WARNING";
        case Level::Error:   return "ERROR";
        case Level::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &nowTime);
#else
    localtime_r(&nowTime, &timeInfo);
#endif
    std::ostringstream timestampStream;
    // Get milliseconds
    auto now_ms = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now_ms.time_since_epoch()
    ) % 1000;

    timestampStream << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    timestampStream << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return timestampStream.str();
}

} // namespace Vkm::Log
