#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <iostream>
#include <string>
#include <map>
#include <ostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <mutex>  // for std::mutex
#include <sstream>

/**
 * @class Logger
 * @brief A simple logging utility that writes messages to separate output streams
 *        based on severity level, with timestamped entries and thread safety.
 */
class Logger {
public:
    /**
     * @enum Level
     * @brief Logging severity levels.
     */
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };

    /**
     * @brief Constructs a Logger with a minimum level and output streams.
     * 
     * @param level Minimum severity level to log. Lower levels are ignored.
     * @param out Output stream for DEBUG and INFO messages (defaults to std::cout).
     * @param err Output stream for WARNING and above (defaults to std::cerr).
     */
    Logger(Level level = Level::DEBUG,
           std::ostream& out = std::cout,
           std::ostream& err = std::cerr)
        : minLevel(level), out(out), err(err) {}

    /**
     * @brief Sets the minimum log level. Messages below this level will be ignored.
     * 
     * @param level The new minimum log level.
     */
    void setLevel(Level level) {
        std::lock_guard<std::mutex> lock(mutex);
        minLevel = level;
    }

    /// Logs a DEBUG level message.
    void debug(const std::string& message)    { log(Level::DEBUG, message); }

    /// Logs an INFO level message.
    void info(const std::string& message)     { log(Level::INFO, message); }

    /// Logs a WARNING level message.
    void warning(const std::string& message)  { log(Level::WARNING, message); }

    /// Logs an ERROR level message.
    void error(const std::string& message)    { log(Level::ERROR, message); }

    /// Logs a CRITICAL level message.
    void critical(const std::string& message) { log(Level::CRITICAL, message); }

private:
    Level minLevel;
    std::ostream& out;
    std::ostream& err;
    std::mutex mutex;  ///< Mutex to ensure thread-safe logging

    const std::map<Level, std::string> levelToString = {
        {Level::DEBUG,    "DEBUG"},
        {Level::INFO,     "INFO"},
        {Level::WARNING,  "WARNING"},
        {Level::ERROR,    "ERROR"},
        {Level::CRITICAL, "CRITICAL"}
    };

    /**
     * @brief Returns the current system time as a formatted string.
     * 
     * @return A string in the format "YYYY-MM-DD HH:MM:SS"
     */
    std::string getCurrentTimestamp() const {
        using std::chrono::system_clock;

        auto now = system_clock::now();
        std::time_t now_c = system_clock::to_time_t(now);

        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &now_c);
#else
        localtime_r(&now_c, &tm_buf);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    /**
     * @brief Checks if the given log level should be output based on current minimum level.
     */
    bool shouldLog(Level level) const {
        return static_cast<int>(level) >= static_cast<int>(minLevel);
    }

    /**
     * @brief Logs a message with timestamp and severity level to the appropriate stream.
     */
    void log(Level level, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex);

        if (!shouldLog(level)) return;

        std::ostream& ostr = (level >= Level::WARNING) ? err : out;
        ostr << "[" << getCurrentTimestamp() << "] "
             << "[" << levelToString.at(level) << "] "
             << message << std::endl;
    }
};

#endif // LOGGER_HPP_
