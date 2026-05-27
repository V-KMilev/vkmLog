#include "logger.h"

#include <filesystem>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cstdarg>
#include <vector>

// Static member initialization
bool Logger::s_isInitialized = false;
std::unique_ptr<Logger> Logger::s_instance = nullptr;

Logger::Logger(
    const std::string& filename,
    const std::string& suffix,
    LogLevel level
) : m_filename(filename),
    m_suffix(suffix),
    m_level(level) {
    // Ensure the log directory exists before opening the log file
    size_t lastSlash = m_filename.find_last_of("/\\");

    if (lastSlash != std::string::npos) {
        std::string dir = m_filename.substr(0, lastSlash);
        std::filesystem::create_directories(dir);
    }

    m_file.open(m_filename, std::ios::app);
}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

bool Logger::init(const std::string& filename, const std::string& suffix, LogLevel level) {
    if (s_isInitialized) {
        printf("WARNING: Logger already initialized! Call Logger::init() only once.\n");
        return false;
    }
    s_isInitialized = true;
    s_instance = std::unique_ptr<Logger>(new Logger(filename, suffix, level));
    return true;
}

Logger& Logger::getInstance() {
    if (!s_instance || !s_isInitialized) {
        printf("ERROR: Logger not initialized! Call Logger::init() before using.\n");
        throw std::runtime_error("Logger not initialized! Call Logger::init() before using.");
    }
    return *s_instance;
}

void Logger::log(LogLevel level, const char* suffix, const char* category, const char* format, ...) {
    if (level < m_level) {
        return;
    }

    if (!m_file.is_open()) {
        printf("WARNING: Failed to open log file: %s\n", m_filename.c_str());
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

        // Output to console
        printf("%s", logStream.str().c_str());

        // Output to log file
        m_file << logStream.str();
        m_file.flush();
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE:   return "TRACE";
        case LogLevel::VERBOSE: return "VERBOSE";
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
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
