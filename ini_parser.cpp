#ifndef INI_PARSER_HPP
#define INI_PARSER_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

/// @brief Top-level namespace for INI parsing library.
namespace ini {

    /// @brief Internal utility namespace for string processing.
    namespace detail {

        /**
         * @brief Trim leading and trailing whitespace from a string.
         *
         * This function modifies the input string in-place by removing
         * any whitespace characters from both the beginning and the end.
         *
         * @param s The string to trim.
         */
        inline void trim(std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
        }

    } // namespace detail

    /**
     * @brief Class to parse and store INI configuration files.
     *
     * This class supports INI files with:
     * - Sections (`[section]`)
     * - Key-value pairs (`key=value`)
     * - Comments starting with `;` or `#` (full-line or inline)
     *
     * Key-value pairs defined before any section header are stored under the empty string (`""`)
     * as the section name, representing global or unscoped configuration.
     *
     * Inline comments are stripped from both section headers and unquoted values after trimming.
     * Values enclosed in single or double quotes (e.g., `key="value with # and ;"`) preserve their contents exactly.
     */
    class Parser {
    public:
        /// @brief A section represented as a key-value mapping of strings.
        using Section = std::unordered_map<std::string, std::string>;

        /// @brief The complete configuration, mapping section names to their corresponding sections.
        using Config = std::unordered_map<std::string, Section>;

        /**
         * @brief Error codes representing the status of INI parsing or operations.
         */
        enum class ErrorCode {
            Success,           ///< Operation completed successfully.
            FileNotFound,      ///< Could not open the specified file.
            InvalidSection,    ///< Found a section with an empty or invalid name.
            InvalidLine,       ///< A line was missing the '=' delimiter.
            EmptyKey,          ///< Found a key that is empty after trimming.
            DuplicateKey,      ///< Encountered a duplicate key in the same section.
            FileWriteFailed    ///< Could not write to the specified file.
        };

        /**
         * @brief Converts an ErrorCode enum value to a human-readable string.
         *
         * @param error The error code to convert.
         * @return A string representing the error.
         */
        static inline std::string errorToString(ErrorCode error) {
            switch (error) {
                case ErrorCode::Success:        return "Success";
                case ErrorCode::FileNotFound:   return "File not found";
                case ErrorCode::InvalidSection: return "Invalid section name";
                case ErrorCode::InvalidLine:    return "Invalid line (missing '=')";
                case ErrorCode::EmptyKey:       return "Empty key";
                case ErrorCode::DuplicateKey:   return "Duplicate key in section";
                case ErrorCode::FileWriteFailed: return "Failed to write to file";
                default:                        return "Unknown error";
            }
        }

        /**
         * @brief Load and parse an INI file from disk.
         *
         * This method supports:
         * - Sections denoted by `[section]`
         * - Key-value pairs (`key=value`)
         * - Comments starting with `;` or `#` (inline or full-line)
         *
         * Key-value pairs defined before any section header are stored under the empty string (`""`)
         * as the section name, representing global or unscoped configuration.
         *
         * Inline comments are stripped from both section headers and unquoted values after trimming.
         * Values enclosed in single or double quotes (e.g., `key="value with # and ;"`) preserve their contents exactly.
         *
         * Parsing stops at the first error encountered.
         *
         * @param filename The path to the INI file.
         * @return An ErrorCode indicating the success or failure reason.
         */
        inline ErrorCode load(const std::string& filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                return ErrorCode::FileNotFound;
            }

            std::string line;
            std::string currentSection;
            size_t lineNumber = 0;

            while (std::getline(file, line)) {
                ++lineNumber;
                detail::trim(line);

                // Skip empty lines and comments
                if (line.empty() || line[0] == ';' || line[0] == '#')
                    continue;

                // Strip inline comments from the entire line (find earliest ';' or '#')
                std::size_t commentPos = std::string::npos;
                for (char commentChar : {';', '#'}) {
                    auto pos = line.find(commentChar);
                    if (pos != std::string::npos && (commentPos == std::string::npos || pos < commentPos)) {
                        commentPos = pos;
                    }
                }
                if (commentPos != std::string::npos) {
                    line = line.substr(0, commentPos);
                    detail::trim(line);
                }

                // If line is now empty after stripping, skip it
                if (line.empty()) continue;

                // Section header
                if (line.front() == '[' && line.back() == ']') {
                    currentSection = line.substr(1, line.length() - 2);
                    detail::trim(currentSection);
                    if (currentSection.empty()) {
                        return ErrorCode::InvalidSection;
                    }
                }
                // Key-value pair
                else {
                    auto pos = line.find('=');
                    if (pos == std::string::npos) {
                        return ErrorCode::InvalidLine;
                    }

                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    detail::trim(key);
                    detail::trim(value);

                    if (key.empty()) {
                        return ErrorCode::EmptyKey;
                    }

                    // Handle quoted values to prevent comment stripping
                    bool isQuoted = (value.size() >= 2) &&
                        ((value.front() == '"' && value.back() == '"') ||
                         (value.front() == '\'' && value.back() == '\''));

                    if (isQuoted) {
                        value = value.substr(1, value.length() - 2); // remove surrounding quotes
                    } else {
                        // Strip inline comments from unquoted values
                        std::size_t commentPos = std::string::npos;
                        for (char commentChar : {';', '#'}) {
                            auto pos = value.find(commentChar);
                            if (pos != std::string::npos && (commentPos == std::string::npos || pos < commentPos)) {
                                commentPos = pos;
                            }
                        }
                        if (commentPos != std::string::npos) {
                            value = value.substr(0, commentPos);
                            detail::trim(value);
                        }
                    }

                    // Detect duplicate key
                    if (config[currentSection].find(key) != config[currentSection].end()) {
                        return ErrorCode::DuplicateKey;
                    }

                    config[currentSection][key] = value;
                }
            }

            return ErrorCode::Success;
        }

        /**
         * @brief Retrieve a value for a given section and key.
         *
         * If the section or key does not exist, returns the specified default value.
         *
         * @param section The section name to search in.
         * @param key The key name to look for.
         * @param defaultValue The value to return if the key is not found.
         * @return The corresponding value, or `defaultValue` if not found.
         */
        inline std::string get(const std::string& section, const std::string& key, const std::string& defaultValue = "") const {
            auto secIt = config.find(section);
            if (secIt != config.end()) {
                auto keyIt = secIt->second.find(key);
                if (keyIt != secIt->second.end()) {
                    return keyIt->second;
                }
            }
            return defaultValue;
        }

        /**
         * @brief Set a key-value pair in the specified section.
         *
         * If the section does not exist, it is created. If the key already exists
         * in the section, it is overwritten. Empty keys are not allowed.
         *
         * @param section The section name to set the key-value pair in.
         * @param key The key to set.
         * @param value The value to associate with the key.
         * @return An ErrorCode indicating the success or failure reason.
         */
        inline ErrorCode set(const std::string& section, const std::string& key, const std::string& value) {
            std::string trimmedKey = key;
            detail::trim(trimmedKey);
            if (trimmedKey.empty()) {
                return ErrorCode::EmptyKey;
            }

            std::string trimmedSection = section;
            detail::trim(trimmedSection);
            if (trimmedSection.find_first_of("[]") != std::string::npos) {
                return ErrorCode::InvalidSection;
            }

            config[trimmedSection][trimmedKey] = value;
            return ErrorCode::Success;
        }

        /**
         * @brief Save the configuration to a file in INI format.
         *
         * Writes all sections and key-value pairs to the specified file.
         * Global key-value pairs (under the empty section "") are written first,
         * followed by named sections. Sections are written in the format `[section]`,
         * and key-value pairs are written as `key=value`. Values containing
         * spaces, semicolons, or hashes are automatically quoted with double quotes.
         *
         * @param filename The path to the file to write.
         * @return An ErrorCode indicating the success or failure reason.
         */
        inline ErrorCode save(const std::string& filename) const {
            std::ofstream file(filename);
            if (!file.is_open()) {
                return ErrorCode::FileWriteFailed;
            }

            // Write global section (empty section name) first
            auto globalIt = config.find("");
            if (globalIt != config.end() && !globalIt->second.empty()) {
                for (const auto& [key, value] : globalIt->second) {
                    // Quote values containing spaces, semicolons, or hashes
                    bool needsQuotes = (value.find_first_of(" ;#") != std::string::npos);
                    file << key << "=";
                    if (needsQuotes) {
                        file << "\"" << value << "\"";
                    } else {
                        file << value;
                    }
                    file << "\n";
                }
                file << "\n"; // Add blank line after global section
            }

            // Write named sections
            for (const auto& [section, pairs] : config) {
                if (section.empty()) continue; // Skip global section
                file << "[" << section << "]\n";
                for (const auto& [key, value] : pairs) {
                    // Quote values containing spaces, semicolons, or hashes
                    bool needsQuotes = (value.find_first_of(" ;#") != std::string::npos);
                    file << key << "=";
                    if (needsQuotes) {
                        file << "\"" << value << "\"";
                    } else {
                        file << value;
                    }
                    file << "\n";
                }
                file << "\n"; // Add blank line after each section
            }

            if (file.fail()) {
                return ErrorCode::FileWriteFailed;
            }

            return ErrorCode::Success;
        }

        /**
         * @brief Access the full parsed configuration data.
         *
         * Provides read-only access to all sections and their key-value pairs.
         *
         * @return A constant reference to the internal configuration map.
         */
        inline const Config& data() const {
            return config;
        }

    private:
        /// @brief Internal storage for the parsed configuration data.
        Config config;
    };

} // namespace ini

#endif //INI_PARSER_HPP


#include <iostream>

int main() {
    ini::Parser parser;

    // Set some key-value pairs
    if (parser.set("", "host", "localhost") != ini::Parser::ErrorCode::Success) {
        std::cerr << "Failed to set global host" << std::endl;
    }
    if (parser.set("database", "user", "admin") != ini::Parser::ErrorCode::Success) {
        std::cerr << "Failed to set database user" << std::endl;
    }
    if (parser.set("database", "password", "secret;pass") != ini::Parser::ErrorCode::Success) {
        std::cerr << "Failed to set database password" << std::endl;
    }

    // Save to a file
    auto result = parser.save("config.ini");
    if (result != ini::Parser::ErrorCode::Success) {
        std::cerr << "Error: " << ini::Parser::errorToString(result) << std::endl;
        return 1;
    }

    // Load and verify
    result = parser.load("config.ini");
    if (result != ini::Parser::ErrorCode::Success) {
        std::cerr << "Error: " << ini::Parser::errorToString(result) << std::endl;
        return 1;
    }

    // Retrieve and print values
    std::cout << "host: " << parser.get("", "host") << std::endl;
    std::cout << "user: " << parser.get("database", "user") << std::endl;
    std::cout << "password: " << parser.get("database", "password") << std::endl;

    return 0;
}

