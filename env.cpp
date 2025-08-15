#ifndef ENV_HPP
#define ENV_HPP

#include <string>
#include <string_view>
#include <optional>
#include <functional>
#include <system_error>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
    #include <Windows.h>
    extern char** _environ; ///< Windows-specific environment variable array
#else
    #if defined(__APPLE__) || defined(__unix__)
        extern char** environ; ///< POSIX environment variable array
    #endif
#endif

namespace env {

    inline std::optional<std::string> get(std::string_view name) {
        if (name.empty() || name.find('=') != std::string_view::npos) {
            return std::nullopt;
        }
        const char* value = std::getenv(name.data());
        if (value) {
            return std::string(value);
        }
        return std::nullopt;
    }

    [[nodiscard]]
    inline std::error_code set(std::string_view name, std::string_view value) {
        if (name.empty() || name.find('=') != std::string_view::npos) {
            return std::make_error_code(std::errc::invalid_argument);
        }
#ifdef _WIN32
        int result = _putenv_s(name.data(), value.data());
        if (result != 0) {
            DWORD winErr = GetLastError();
            return std::error_code(static_cast<int>(winErr), std::system_category());
        }
#else
        if (setenv(name.data(), value.data(), 1) != 0) {
            return std::error_code(errno, std::generic_category());
        }
#endif
        return {};
    }

    [[nodiscard]]
    inline std::error_code unset(std::string_view name) {
        if (name.empty() || name.find('=') != std::string_view::npos) {
            return std::make_error_code(std::errc::invalid_argument);
        }
#ifdef _WIN32
        int result = _putenv_s(name.data(), "");
        if (result != 0) {
            DWORD winErr = GetLastError();
            return std::error_code(static_cast<int>(winErr), std::system_category());
        }
#else
        if (unsetenv(name.data()) != 0) {
            return std::error_code(errno, std::generic_category());
        }
#endif
        return {};
    }

    inline void each(const std::function<void(std::string_view, std::string_view)>& fn) {
#ifdef _WIN32
        char** env = _environ;
#else
        char** env = environ;
#endif
        while (*env != nullptr) {
            std::string_view entry(*env);
            auto pos = entry.find('=');
            if (pos != std::string_view::npos && pos > 0) {
                std::string_view name = entry.substr(0, pos);
                std::string_view value = (pos + 1 < entry.size()) ? entry.substr(pos + 1) : "";
                fn(name, value);
            }
            ++env;
        }
    }

} // namespace env

#endif // ENV_HPP

#include <iostream>

int main() {
    std::cout << "Checking for APP_CONFIG...\n";
    if (auto config_path = env::get("APP_CONFIG")) {
        std::cout << "Configuration file path: " << *config_path << "\n";
    } else {
        std::cout << "APP_CONFIG not set. Using default configuration.\n";
    }

    std::cout << "\nChecking APP_LOG_LEVEL...\n";
    auto log_level = env::get("APP_LOG_LEVEL");
    if (!log_level) {
        auto result = env::set("APP_LOG_LEVEL", "INFO");
        if (result) {
            std::cerr << "Failed to set APP_LOG_LEVEL: " << result.message() << "\n";
            return 1;
        }
        std::cout << "Set APP_LOG_LEVEL to INFO\n";
    } else {
        std::cout << "APP_LOG_LEVEL already set: " << *log_level << "\n";
    }

    std::cout << "\nRemoving deprecated OLD_CONFIG...\n";
    auto result = env::unset("OLD_CONFIG");
    if (result) {
        std::cerr << "Failed to unset OLD_CONFIG: " << result.message() << "\n";
    } else {
        std::cout << "Successfully unset OLD_CONFIG\n";
    }

    std::cout << "\nListing all APP_* environment variables:\n";
    env::each([](std::string_view key, std::string_view value) {
        if (key.size() >= 4 && key.substr(0, 4) == "APP_") {
            std::cout << key << "=" << value << "\n";
        }
    });

    std::cout << "\nTesting invalid input...\n";
    result = env::set("INVALID=NAME", "value");
    if (result == std::errc::invalid_argument) {
        std::cout << "Correctly rejected invalid variable name 'INVALID=NAME'\n";
    } else {
        std::cerr << "Unexpected error when setting invalid name: " << result.message() << "\n";
        return 1;
    }

    return 0;
}
