#pragma once
#include <fmt/format.h>

#include <filesystem>
#include <stdexcept>
#include <string_view>

#include "Core/Macros.h"
namespace Voluma {

class VL_API Logger {
   public:
    enum class Level : int {
        Disabled = 0,  // disabled logger, namely suppress all output.
        Fatal = 1,     // fatal error message, which will terminate the program.
        Error = 2,     // error message.
        Warning = 3,   // warning message.
        Info = 4,      // normal information output.
        Debug = 5,     // verbose message for debugging.
        Count = 6
    };
    struct LoggerConfig {
        std::filesystem::path logPath;  // TODO: multiple log files.
        Level logLevel{Level::Info};
        bool logToStdout{true};
        LoggerConfig() = default;
        LoggerConfig(std::filesystem::path logPath, Level logLevel,
                     bool logToStdout)
            : logPath(logPath), logLevel(logLevel), logToStdout(logToStdout) {}
    };

    static void init(const LoggerConfig& config);
    static void log(Level level, const std::string_view message);
    static void shutdown();

    static Level getLogLevel();
    static void setLogLevel(Level level);

    /**
     * @brief Get bool value indicating whether log to stdout.
     *
     * @return true
     * @return false
     */
    static bool getLogToStdout();
    /**
     * @brief Set bool value indicating whether log to stdout.
     *
     * @param logToStdout
     */
    static void setLogToStdout(bool logToStdout);

   private:
    Logger(const LoggerConfig& config);

    Level mLogLevel;    ///< Logger output level
    bool mLogToStdout;  ///< Enable stdout console logging
};

void logBeforeInitialized(Logger::Level level, const std::string_view message);

template <typename... Args>
inline void logBeforeInitialized(Logger::Level level,
                                 fmt::format_string<Args...> format,
                                 Args&&... args) {
    logBeforeInitialized(level,
                         fmt::format(format, std::forward<Args>(args)...));
}

template <typename... Args>
inline void logDebug(fmt::format_string<Args...> format, Args&&... args) {
    Logger::log(Logger::Level::Debug,
                fmt::format(format, std::forward<Args>(args)...));
}
inline void logDebug(const std::string_view message) {
    Logger::log(Logger::Level::Debug, message);
}

template <typename... Args>
inline void logInfo(fmt::format_string<Args...> format, Args&&... args) {
    Logger::log(Logger::Level::Info,
                fmt::format(format, std::forward<Args>(args)...));
}
inline void logInfo(const std::string_view message) {
    Logger::log(Logger::Level::Info, message);
}

template <typename... Args>
inline void logWarning(fmt::format_string<Args...> format, Args&&... args) {
    Logger::log(Logger::Level::Warning,
                fmt::format(format, std::forward<Args>(args)...));
}
inline void logWarning(const std::string_view message) {
    Logger::log(Logger::Level::Warning, message);
}

template <typename... Args>
inline void logError(fmt::format_string<Args...> format, Args&&... args) {
    Logger::log(Logger::Level::Error,
                fmt::format(format, std::forward<Args>(args)...));
}
inline void logError(const std::string_view message) {
    Logger::log(Logger::Level::Error, message);
}

template <typename... Args>
[[noreturn]] inline void logFatal(fmt::format_string<Args...> format,
                                  Args&&... args) {
    const auto& message = fmt::format(format, std::forward<Args>(args)...);
    Logger::log(Logger::Level::Fatal, message);
    throw std::runtime_error(message);
}

[[noreturn]] inline void logFatal(const std::string_view message) {
    Logger::log(Logger::Level::Fatal, message);
    throw std::runtime_error(message.data());
}

}  // namespace Voluma