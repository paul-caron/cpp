#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <iostream>
#include <string>
#include <unordered_map>
#include <ostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <sstream>

/**
 * @class Logger
 * @brief A thread-safe logging utility that writes timestamped messages to separate output streams
 *        based on severity level.
 *
 * The Logger class supports five severity levels (DEBUG, INFO, WARNING, ERROR, CRITICAL) and
 * directs messages to configurable output streams. Messages below the minimum severity level
 * are ignored. The logger is thread-safe, using a mutex to protect stream access.
 *
 * @note Requires C++17 or later for inline static variables.
 * @note Users must ensure that the provided output streams remain valid for the logger's lifetime.
 */
class Logger {
public:
    /**
     * @enum Level
     * @brief Logging severity levels, ordered by increasing severity.
     */
    enum class Level {
        DEBUG = 0,    ///< Detailed diagnostic information for debugging.
        INFO = 1,     ///< General information about program execution.
        WARNING = 2,  ///< Potential issues that do not prevent execution.
        ERROR = 3,    ///< Errors that impact functionality but allow continuation.
        CRITICAL = 4  ///< Severe errors that may cause program termination.
    };

    /**
     * @brief Constructs a Logger with a minimum severity level and output streams.
     *
     * @param level Minimum severity level to log (default: DEBUG). Messages below this level are ignored.
     * @param out Output stream for DEBUG and INFO messages (default: std::cout).
     * @param err Output stream for WARNING, ERROR, and CRITICAL messages (default: std::cerr).
     *
     * @note The provided streams must remain valid for the logger's lifetime to avoid undefined behavior.
     */
    Logger(Level level = Level::DEBUG,
           std::ostream& out = std::cout,
           std::ostream& err = std::cerr)
        : minLevel(level), out(out), err(err) {}

    /**
     * @brief Sets the minimum severity level for logging.
     *
     * Messages below the specified level will be ignored. This method is thread-safe.
     *
     * @param level The new minimum severity level.
     */
    void setLevel(Level level) {
        std::lock_guard<std::mutex> lock(mutex);
        minLevel = level;
    }

    /**
     * @brief Logs a message at the DEBUG severity level.
     *
     * The message is written to the `out` stream with a timestamp and severity tag if
     * the current minimum level is DEBUG or lower.
     *
     * @param message The message to log.
     */
    void debug(const std::string& message) { log(Level::DEBUG, message); }

    /**
     * @brief Logs a message at the INFO severity level.
     *
     * The message is written to the `out` stream with a timestamp and severity tag if
     * the current minimum level is INFO or lower.
     *
     * @param message The message to log.
     */
    void info(const std::string& message) { log(Level::INFO, message); }

    /**
     * @brief Logs a message at the WARNING severity level.
     *
     * The message is written to the `err` stream with a timestamp and severity tag if
     * the current minimum level is WARNING or lower.
     *
     * @param message The message to log.
     */
    void warning(const std::string& message) { log(Level::WARNING, message); }

    /**
     * @brief Logs a message at the ERROR severity level.
     *
     * The message is written to the `err` stream with a timestamp and severity tag if
     * the current minimum level is ERROR or lower.
     *
     * @param message The message to log.
     */
    void error(const std::string& message) { log(Level::ERROR, message); }

    /**
     * @brief Logs a message at the CRITICAL severity level.
     *
     * The message is written to the `err` stream with a timestamp and severity tag if
     * the current minimum level is CRITICAL or lower.
     *
     * @param message The message to log.
     */
    void critical(const std::string& message) { log(Level::CRITICAL, message); }

private:
    Level minLevel;             ///< Minimum severity level for logging.
    std::ostream& out;          ///< Output stream for DEBUG and INFO messages.
    std::ostream& err;          ///< Output stream for WARNING, ERROR, and CRITICAL messages.
    mutable std::mutex mutex;   ///< Mutex to ensure thread-safe logging.

    /**
     * @brief Mapping of log levels to their string representations.
     *
     * @note Requires C++17 or later for inline static variables.
     */
    inline static const std::unordered_map<Level, std::string> levelToString = {
        {Level::DEBUG,    "DEBUG"},
        {Level::INFO,     "INFO"},
        {Level::WARNING,  "WARNING"},
        {Level::ERROR,    "ERROR"},
        {Level::CRITICAL, "CRITICAL"}
    };

    /**
     * @brief Returns the current system time as a formatted string.
     *
     * The timestamp is in the format "YYYY-MM-DD HH:MM:SS.MS" using local time.
     *
     * @return A string representing the current timestamp.
     */
    std::string getCurrentTimestamp() const {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::time_t now_c = system_clock::to_time_t(now);

        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &now_c);
#else
        localtime_r(&now_c, &tm_buf);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    /**
     * @brief Checks if a log level should be output based on the current minimum level.
     *
     * @param level The severity level to check.
     * @return True if the level is at or above the minimum level, false otherwise.
     */
    bool shouldLog(Level level) const {
        return static_cast<int>(level) >= static_cast<int>(minLevel);
    }

    /**
     * @brief Logs a message with a timestamp and severity level to the appropriate stream.
     *
     * The message is written to the `out` stream for DEBUG and INFO levels, or the `err`
     * stream for WARNING, ERROR, and CRITICAL levels. The output includes a timestamp
     * and severity tag. This method is thread-safe.
     *
     * @param level The severity level of the message.
     * @param message The message to log.
     */
    void log(Level level, const std::string& message) const {
        if (!shouldLog(level)) return;
        std::lock_guard<std::mutex> lock(mutex);
        std::ostream& ostr = (level >= Level::WARNING) ? err : out;
        if (!ostr.good()) {
            // Fallback to std::cerr if the intended stream is bad
            std::cerr << "[Logger ERROR] Output stream for level "
                  << levelToString.at(level)
                  << " is in a bad state. Failed to log message: "
                  << message << std::endl;
            std::cerr.flush();
            return;
        }

        ostr << "[" << getCurrentTimestamp() << "] "
             << "[" << levelToString.at(level) << "] "
             << message << std::endl;
        ostr.flush();
    }
};

#endif // LOGGER_HPP_

