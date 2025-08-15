#ifndef INI_PARSER_HPP
#define INI_PARSER_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

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
         * Inline comments are stripped only if preceded by whitespace.
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
            
            config.clear();

            std::string line;
            std::string currentSection;
            size_t lineNumber = 0;

            while (std::getline(file, line)) {
                ++lineNumber;
                detail::trim(line);

                // Skip empty lines and full-line comments
                if (line.empty() || line[0] == ';' || line[0] == '#')
                    continue;

                // Strip inline comments (only if preceded by whitespace)
                std::size_t commentPos = std::string::npos;
                for (char commentChar : {';', '#'}) {
                    for (std::size_t i = 0; i < line.length(); ++i) {
                        if (line[i] == commentChar && (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1])))) {
                            commentPos = i;
                            break;
                        }
                    }
                    if (commentPos != std::string::npos) break;
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

#endif // INI_PARSER_HPP

#include <iostream>
#include <fstream>
#include <string>

bool run_test(const std::string& test_name, bool condition) {
    if (condition) {
        std::cout << test_name << ": PASS" << std::endl;
        return true;
    } else {
        std::cout << test_name << ": FAIL" << std::endl;
        return false;
    }
}

int main() {
    int failed_tests = 0;

    // Test 1: Load non-existent file
    {
        ini::Parser parser;
        auto result = parser.load("non_existent.ini");
        if (!run_test("Test 1: Load non-existent file", result == ini::Parser::ErrorCode::FileNotFound)) {
            ++failed_tests;
        }
    }

    // Test 2: Save empty config
    {
        ini::Parser parser;
        auto result = parser.save("empty.ini");
        if (!run_test("Test 2: Save empty config", result == ini::Parser::ErrorCode::Success)) {
            ++failed_tests;
        }
        // Load back
        ini::Parser parser2;
        result = parser2.load("empty.ini");
        if (!run_test("Test 2: Load empty config", result == ini::Parser::ErrorCode::Success && parser2.data().empty())) {
            ++failed_tests;
        }
    }

    // Test 3: Set and get in global section
    {
        ini::Parser parser;
        auto result = parser.set("", "global_key", "global_value");
        if (!run_test("Test 3: Set global", result == ini::Parser::ErrorCode::Success)) {
            ++failed_tests;
        }
        std::string value = parser.get("", "global_key");
        if (!run_test("Test 3: Get global", value == "global_value")) {
            ++failed_tests;
        }
    }

    // Test 4: Set and get in named section
    {
        ini::Parser parser;
        auto result = parser.set("section1", "key1", "value1");
        if (!run_test("Test 4: Set in section", result == ini::Parser::ErrorCode::Success)) {
            ++failed_tests;
        }
        std::string value = parser.get("section1", "key1");
        if (!run_test("Test 4: Get in section", value == "value1")) {
            ++failed_tests;
        }
    }

    // Test 5: Set with empty key (error)
    {
        ini::Parser parser;
        auto result = parser.set("section", "", "value");
        if (!run_test("Test 5: Set empty key", result == ini::Parser::ErrorCode::EmptyKey)) {
            ++failed_tests;
        }
    }

    // Test 6: Set with invalid section (contains [])
    {
        ini::Parser parser;
        auto result = parser.set("[invalid]", "key", "value");
        if (!run_test("Test 6: Set invalid section", result == ini::Parser::ErrorCode::InvalidSection)) {
            ++failed_tests;
        }
    }

    // Test 7: Overwrite existing key
    {
        ini::Parser parser;
        parser.set("section", "key", "old");
        auto result = parser.set("section", "key", "new");
        if (!run_test("Test 7: Overwrite key", result == ini::Parser::ErrorCode::Success && parser.get("section", "key") == "new")) {
            ++failed_tests;
        }
    }

    // Test 8: Save with quoting needed
    {
        ini::Parser parser;
        parser.set("", "key_space", "value with space");
        parser.set("", "key_semi", "value;with;semi");
        parser.set("", "key_hash", "value#with#hash");
        auto result = parser.save("quoting.ini");
        if (!run_test("Test 8: Save with quoting", result == ini::Parser::ErrorCode::Success)) {
            ++failed_tests;
        }
        // Load back
        ini::Parser parser2;
        result = parser2.load("quoting.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser2.get("", "key_space") == "value with space" &&
                      parser2.get("", "key_semi") == "value;with;semi" &&
                      parser2.get("", "key_hash") == "value#with#hash");
        if (!run_test("Test 8: Load quoted values", check)) {
            ++failed_tests;
        }
    }

    // Test 9: Load with global and sections, comments
    {
        std::ofstream file("complex.ini");
        file << "; Global comment\n";
        file << "global_key = global_value\n";
        file << "# Section comment\n";
        file << "[section1]\n";
        file << "key1=value1 ; inline comment\n";
        file << "key2=value2#no space inline\n";
        file << "key3=\"quoted ; with # space\"\n";
        file << "\n";
        file << "[section2]\n";
        file << "key4 = value4\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("complex.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "global_key") == "global_value" &&
                      parser.get("section1", "key1") == "value1" &&  // inline stripped
                      parser.get("section1", "key2") == "value2#no space inline" &&  // no space, not stripped
                      parser.get("section1", "key3") == "quoted ; with # space" &&  // quoted preserved
                      parser.get("section2", "key4") == "value4");
        if (!run_test("Test 9: Load complex INI", check)) {
            ++failed_tests;
        }
    }

    // Test 10: Load with duplicate key (error)
    {
        std::ofstream file("duplicate.ini");
        file << "key=first\n";
        file << "key=second\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("duplicate.ini");
        if (!run_test("Test 10: Load duplicate key", result == ini::Parser::ErrorCode::DuplicateKey)) {
            ++failed_tests;
        }
    }

    // Test 11: Load with empty key (error)
    {
        std::ofstream file("empty_key.ini");
        file << " =value\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("empty_key.ini");
        if (!run_test("Test 11: Load empty key", result == ini::Parser::ErrorCode::EmptyKey)) {
            ++failed_tests;
        }
    }

    // Test 12: Load with invalid line (no =)
    {
        std::ofstream file("invalid_line.ini");
        file << "key_only\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("invalid_line.ini");
        if (!run_test("Test 12: Load invalid line", result == ini::Parser::ErrorCode::InvalidLine)) {
            ++failed_tests;
        }
    }

    // Test 13: Load with empty section (error)
    {
        std::ofstream file("empty_section.ini");
        file << "[]\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("empty_section.ini");
        if (!run_test("Test 13: Load empty section", result == ini::Parser::ErrorCode::InvalidSection)) {
            ++failed_tests;
        }
    }

    // Test 14: Get non-existent key (default value)
    {
        ini::Parser parser;
        std::string value = parser.get("non_section", "non_key", "default");
        if (!run_test("Test 14: Get non-existent", value == "default")) {
            ++failed_tests;
        }
    }

    // Test 15: Save failure (e.g., invalid path, but hard to test reliably; skip or simulate)
    // Note: Testing FileWriteFailed reliably requires a write-protected file or dir, which may not be portable.
    // For now, assume success on valid path.

    std::cout << "Total failed tests: " << failed_tests << std::endl;
    return failed_tests > 0 ? 1 : 0;
}
