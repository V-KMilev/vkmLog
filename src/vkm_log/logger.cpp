#include "vkm_log/logger.h"

#include <array>
#include <chrono>
#include <cstdarg>
#include <cstdio>
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

    m_file.open(path, std::ios::app);
    if (!m_file.is_open()) return;

    m_path     = path;
    m_runStart = std::filesystem::file_size(path, ec);
    if (ec) m_runStart = 0;
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

        // Flushed: piped to a log collector, stdout is fully buffered, and a headless process would
        // show nothing until it exits.
        printf("%s", line.c_str());
        fflush(stdout);

        if (m_file.is_open()) {
            m_file << line;
            m_file.flush();
        }
    }
}

std::string_view levelName(Level level) {
    const size_t index = static_cast<size_t>(level);
    return index < std::size(LEVEL_NAMES) ? LEVEL_NAMES[index] : "UNKNOWN";
}

std::optional<Line> parseLine(std::string_view text) {
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
