#include "logger.h"

#include <fmt/chrono.h>

#include <chrono>
#include <cstdio>
#include <mutex>

#include "core/error.h"

namespace Voluma {
static std::mutex loggerMutex;  // mutex for thread-safe logging.
static Logger* pLogger;         // singleton logger.
static FILE* pFd{nullptr};      // file descriptor for log file.

constexpr std::string_view s_timeFormat = "{:%T}";
constexpr std::string_view s_logFormat = "[{level:<7} {time:>16}] {message}\n";
Logger::Logger(const LoggerConfig& config)
    : mLogLevel(config.logLevel), mLogToStdout(config.logToStdout) {}

static std::string_view getLevelName(Logger::Level level) {
    switch (level) {
        case Logger::Level::Debug:
            return "Debug";
        case Logger::Level::Info:
            return "Info";
        case Logger::Level::Warning:
            return "Warning";
        case Logger::Level::Error:
            return "Error";
        case Logger::Level::Fatal:
            return "Fatal";
        default:
            return "Unknown";
    }
}

static std::string formatLogMessage(
    std::chrono::time_point<std::chrono::system_clock> time,
    Logger::Level level, std::string_view message) {
    return fmt::format(s_logFormat, fmt::arg("level", getLevelName(level)),
                       fmt::arg("time", fmt::format(s_timeFormat, time)),
                       fmt::arg("message", message));
}

static void writeToFile(FILE* fd, std::string_view message) {
    VL_ASSERT(fd != nullptr);
    std::fprintf(fd, "%s", message.data());
    // flush immediately to avoid losing log messages.
    std::fflush(fd);
}
/**
 * @brief print log message to stdout before logger initialization.
 */
void logBeforeInitialized(Logger::Level level, std::string_view message) {
    auto time = std::chrono::system_clock::now();
    writeToFile(stdout, formatLogMessage(time, level, message));
    if (level == Logger::Level::Fatal) std::exit(1);
}

void Logger::init(const LoggerConfig& config) {
    pLogger = new Logger(config);

    const auto logPath = config.logPath;
    if (!logPath.empty()) {
        pFd = std::fopen(logPath.string().c_str(), "w");
        if (pFd == nullptr) {
            logBeforeInitialized(Logger::Level::Fatal, std::strerror(errno));
            std::exit(1);
        }
    }
}

void Logger::log(Level level, std::string_view message) {
    if (pLogger == nullptr) {
        logBeforeInitialized(Logger::Level::Fatal, "Logger not initialized.");
        return;
    }
    if (static_cast<int>(level) > static_cast<int>(pLogger->mLogLevel)) {
        return;
    }
    std::string formattedMessage(
        formatLogMessage(std::chrono::system_clock::now(), level, message));
    std::lock_guard<std::mutex> lock(loggerMutex);
    if (pLogger->mLogToStdout) {
        writeToFile(stdout, formattedMessage);
    }
    if (pFd != nullptr) {
        writeToFile(pFd, formattedMessage);
    }
}

void Logger::shutdown() {
    delete pLogger;
    if (pFd != nullptr) {
        fclose(pFd);
        pFd = nullptr;
    }
}

using Level = Logger::Level;
Level Logger::getLogLevel() { return pLogger->mLogLevel; }
void Logger::setLogLevel(Level level) { pLogger->mLogLevel = level; }

bool Logger::getLogToStdout() { return pLogger->mLogToStdout; }

}  // namespace Voluma
