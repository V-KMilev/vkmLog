#include "vkm_log/logger.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Vkm::Log {

namespace {

constexpr std::string_view LEVEL_NAMES[] = {
    "TRACE",
    "VERBOSE",
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL"
};  ///< In Level's order.
static_assert(std::size(LEVEL_NAMES) == static_cast<size_t>(Level::Count), "a name for each level");

/// A line on the console, flushed: piped to a log collector, stdout is fully buffered, and a
/// headless process would show nothing until it exits.
VKM_LOG_PRINTF(1, 2) void writeConsole(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

/// Now, local time, as "YYYY-MM-DD HH:MM:SS.mmm".
std::string timestamp() {
    auto now     = std::chrono::system_clock::now();
    auto nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm timeInfo;
#ifdef _WIN32
    localtime_s(&timeInfo, &nowTime);
#else
    localtime_r(&nowTime, &timeInfo);
#endif
    std::ostringstream timestampStream;
    // From the same reading as the seconds: a second reading may have crossed into the next second.
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    timestampStream << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
    timestampStream << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return timestampStream.str();
}

} // namespace

std::unique_ptr<Logger> Logger::s_instance = nullptr;

Logger::Logger(const std::filesystem::path& path, const std::string& suffix, Level level)
    : m_suffix(suffix)
    , m_level(level)
{
    // A folder that cannot be made shows as a file that will not open.
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    // Binary: a line ends in '\n' alone on every platform, as parseLine and a reader of the file expect.
    m_file.open(path, std::ios::app | std::ios::binary);
    if (!m_file.is_open()) {
        const char* reason = std::strerror(errno);
        writeConsole("WARNING: Logger cannot open %s (%s).\n", path.string().c_str(), reason);
        return;
    }

    m_path     = path;
    m_runStart = std::filesystem::file_size(path, ec);
    if (ec) m_runStart = 0;
}

bool Logger::init(const std::filesystem::path& path, const std::string& suffix, Level level) {
    if (s_instance && s_instance->m_file.is_open()) {
        writeConsole("WARNING: Logger already has a file; Logger::init() is refused.\n");
        return false;
    }
    s_instance = std::unique_ptr<Logger>(new Logger(path, suffix, level));
    return s_instance->m_file.is_open();
}

Logger& Logger::getInstance() {
    if (!s_instance) {
        writeConsole("ERROR: Logger not initialized! Call Logger::init() before using.\n");
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
        writeConsole("ERROR: Logger cannot format \"%s\"; the line is dropped.\n", format);
        return;
    }

    if (static_cast<size_t>(len) >= buffer.size()) {
        buffer.resize(len + 1);
        va_start(args, format);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);
        va_end(args);
    }

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        std::ostringstream logStream;
        // The form parseLine reads.
        const char* eff = (suffix && *suffix) ? suffix : m_suffix.c_str();
        logStream << "[" << timestamp() << "] [" << eff << "]";
        if (category && *category) {
            logStream << " [" << category << "]";
        }
        logStream << " [" << levelName(level) << "] " << buffer.data() << "\n";
        const std::string line = logStream.str();

        writeConsole("%s", line.c_str());

        // A stream that failed once stays failed: the file is said once, and kept, so init is still refused.
        if (m_file.is_open() && m_file.good()) {
            m_file << line;
            m_file.flush();
            if (!m_file) {
                const char* reason = std::strerror(errno);
                writeConsole(
                    "ERROR: Logger cannot write to %s (%s); lines go to the console alone.\n",
                    m_path.string().c_str(),
                    reason
                );
            }
        }
    }
}

std::string_view levelName(Level level) {
    const size_t index = static_cast<size_t>(level);
    return index < std::size(LEVEL_NAMES) ? LEVEL_NAMES[index] : "UNKNOWN";
}

std::optional<Line> parseLine(std::string_view text) {
    // A file another tool wrote may end its lines in "\r\n".
    if (!text.empty() && text.back() == '\r') text.remove_suffix(1);

    // The bracketed fields at the front, up to four: time, suffix, category, level.
    constexpr size_t MAX_FIELDS = 4;

    std::array<std::string_view, MAX_FIELDS> fields;
    std::array<size_t, MAX_FIELDS>           after{};  // Where the text past each field starts.
    size_t count = 0;
    size_t at    = 0;
    while (count < MAX_FIELDS && at < text.size() && text[at] == '[') {
        const size_t close = text.find(']', at);
        if (close == std::string_view::npos) break;
        fields[count] = text.substr(at + 1, close - at - 1);
        at            = close + 1;
        if (at < text.size() && text[at] == ' ') ++at;
        after[count++] = at;
    }

    // The category is optional, so the level is the third field or the fourth.
    for (size_t field = 2; field < count; ++field) {
        for (size_t index = 0; index < std::size(LEVEL_NAMES); ++index) {
            if (fields[field] != LEVEL_NAMES[index]) continue;
            Line line;
            line.time     = fields[0];
            line.suffix   = fields[1];
            line.category = field == 3 ? fields[2] : std::string_view();
            line.level    = static_cast<Level>(index);
            line.message  = text.substr(after[field]);
            return line;
        }
    }
    return std::nullopt;
}

} // namespace Vkm::Log
