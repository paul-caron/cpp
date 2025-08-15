#ifndef INI_PARSER_HPP
#define INI_PARSER_HPP

#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace ini {
    namespace detail {
        inline void trim(std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
        }

        inline std::string unescape(const std::string& s) {
            std::string result;
            result.reserve(s.size());
            bool escaped = false;
            for (size_t i = 0; i < s.size(); ++i) {
                if (escaped) {
                    switch (s[i]) {
                        case '"': result += '"'; break;
                        case '\'': result += '\''; break;
                        case '\\': result += '\\'; break;
                        case 'n': result += '\n'; break;
                        case 't': result += '\t'; break;
                        case 'r': result += '\r'; break;
                        default: result += s[i]; break;
                    }
                    escaped = false;
                } else if (s[i] == '\\') {
                    escaped = true;
                } else {
                    result += s[i];
                }
            }
            return result;
        }

        inline std::string escape(const std::string& s) {
            std::string result;
            result.reserve(s.size() * 2);
            for (char c : s) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\'': result += "\\\'"; break;
                    case '\\': result += "\\\\"; break;
                    case '\n': result += "\\n"; break;
                    case '\t': result += "\\t"; break;
                    case '\r': result += "\\r"; break;
                    default: result += c; break;
                }
            }
            return result;
        }
    }

    class Parser {
    public:
        using Section = std::unordered_map<std::string, std::string>;
        using Config = std::unordered_map<std::string, Section>;

        enum class ErrorCode {
            Success,
            FileNotFound,
            InvalidSection,
            InvalidLine,
            EmptyKey,
            DuplicateKey,
            FileWriteFailed,
            UnmatchedQuotes  // <-- NEW
        };

        static inline std::string errorToString(ErrorCode error) {
            switch (error) {
                case ErrorCode::Success: return "Success";
                case ErrorCode::FileNotFound: return "File not found";
                case ErrorCode::InvalidSection: return "Invalid section name";
                case ErrorCode::InvalidLine: return "Invalid line (missing '=')";
                case ErrorCode::EmptyKey: return "Empty key";
                case ErrorCode::DuplicateKey: return "Duplicate key in section";
                case ErrorCode::FileWriteFailed: return "Failed to write to file";
                case ErrorCode::UnmatchedQuotes: return "Unmatched quotes in value"; // NEW
                default: return "Unknown error";
            }
        }

        explicit Parser(bool strict_comments = true) : strict_comments_(strict_comments) {}

        inline ErrorCode load(const std::string& filename) {
            std::ifstream file(filename);
            if (!file.is_open()) return ErrorCode::FileNotFound;

            config.clear();
            std::string line;
            std::string currentSection;

            while (std::getline(file, line)) {
                detail::trim(line);
                if (line.empty() || line[0] == ';' || line[0] == '#') continue;

                bool inside_single_quote = false;
                bool inside_double_quote = false;
                std::size_t commentPos = std::string::npos;

                for (std::size_t i = 0; i < line.length(); ++i) {
                    char ch = line[i];
                    if (ch == '"' && !inside_single_quote) inside_double_quote = !inside_double_quote;
                    else if (ch == '\'' && !inside_double_quote) inside_single_quote = !inside_single_quote;
                    if ((ch == ';' || ch == '#') && !inside_single_quote && !inside_double_quote) {
                        if (strict_comments_ && i > 0 && !std::isspace(static_cast<unsigned char>(line[i - 1]))) continue;
                        commentPos = i;
                        break;
                    }
                }

                if (commentPos != std::string::npos) {
                    line = line.substr(0, commentPos);
                    detail::trim(line);
                }

                if (line.empty()) continue;

                if (line.front() == '[' && line.back() == ']') {
                    currentSection = line.substr(1, line.length() - 2);
                    detail::trim(currentSection);
                    if (currentSection.empty()) return ErrorCode::InvalidSection;
                } else {
                    auto pos = line.find('=');
                    if (pos == std::string::npos) return ErrorCode::InvalidLine;

                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    detail::trim(key);
                    detail::trim(value);

                    if (key.empty()) return ErrorCode::EmptyKey;

                    if (value.size() >= 1 && (value.front() == '"' || value.front() == '\'')) {
                        char quote = value.front();
                        if (value.size() < 2 || value.back() != quote) {
                            return ErrorCode::UnmatchedQuotes;
                        }
                        value = detail::unescape(value.substr(1, value.length() - 2));
                    }

                    if (config[currentSection].find(key) != config[currentSection].end()) {
                        return ErrorCode::DuplicateKey;
                    }

                    config[currentSection][key] = value;
                }
            }

            return ErrorCode::Success;
        }

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

        inline ErrorCode set(const std::string& section, const std::string& key, const std::string& value) {
            std::string trimmedKey = key;
            detail::trim(trimmedKey);
            if (trimmedKey.empty()) return ErrorCode::EmptyKey;

            std::string trimmedSection = section;
            detail::trim(trimmedSection);
            if (trimmedSection.find_first_of("[]") != std::string::npos) return ErrorCode::InvalidSection;

            config[trimmedSection][trimmedKey] = value;
            return ErrorCode::Success;
        }

        inline ErrorCode save(const std::string& filename) const {
            std::ofstream file(filename);
            if (!file.is_open()) return ErrorCode::FileWriteFailed;

            auto globalIt = config.find("");
            if (globalIt != config.end() && !globalIt->second.empty()) {
                for (const auto& [key, value] : globalIt->second) {
                    bool needsQuotes = (value.find_first_of(" ;#\"\n\t\r") != std::string::npos);
                    file << key << "=";
                    if (needsQuotes) {
                        file << "\"" << detail::escape(value) << "\"";
                    } else {
                        file << value;
                    }
                    file << "\n";
                }
                file << "\n";
            }

            for (const auto& [section, pairs] : config) {
                if (section.empty()) continue;
                file << "[" << section << "]\n";
                for (const auto& [key, value] : pairs) {
                    bool needsQuotes = (value.find_first_of(" ;#\"\n\t\r") != std::string::npos);
                    file << key << "=";
                    if (needsQuotes) {
                        file << "\"" << detail::escape(value) << "\"";
                    } else {
                        file << value;
                    }
                    file << "\n";
                }
                file << "\n";
            }

            return file.fail() ? ErrorCode::FileWriteFailed : ErrorCode::Success;
        }

        inline const Config& data() const {
            return config;
        }

    private:
        Config config;
        bool strict_comments_;
    };
}

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
                      parser.get("section1", "key1") == "value1" &&
                      parser.get("section1", "key2") == "value2#no space inline" &&
                      parser.get("section1", "key3") == "quoted ; with # space" &&
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

    // Test 15: Non-spaced inline comments (relaxed mode)
    {
        std::ofstream file("non_spaced.ini");
        file << "key1=value1;comment\n";
        file << "key2=value2#comment\n";
        file.close();

        ini::Parser parser(false); // Relaxed comment parsing
        auto result = parser.load("non_spaced.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "key1") == "value1" &&
                      parser.get("", "key2") == "value2");
        if (!run_test("Test 15: Non-spaced inline comments (relaxed)", check)) {
            ++failed_tests;
        }
    }

    // Test 16: Escaped values
    {
        std::ofstream file("escaped.ini");
        file << "key1=\"value \\\"with\\\" quote\\nline\"\n";
        file << "key2='escaped \\'single\\' quote\\t'\n";
        file << "key3=\"backslash \\\\ here\"\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("escaped.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "key1") == "value \"with\" quote\nline" &&
                      parser.get("", "key2") == "escaped 'single' quote\t" &&
                      parser.get("", "key3") == "backslash \\ here");
        if (!run_test("Test 16: Escaped values", check)) {
            ++failed_tests;
        }
        // Round-trip test
        parser.save("escaped_roundtrip.ini");
        ini::Parser parser2;
        result = parser2.load("escaped_roundtrip.ini");
        check = (result == ini::Parser::ErrorCode::Success &&
                 parser2.get("", "key1") == "value \"with\" quote\nline" &&
                 parser2.get("", "key2") == "escaped 'single' quote\t" &&
                 parser2.get("", "key3") == "backslash \\ here");
        if (!run_test("Test 16: Escaped values round-trip", check)) {
            ++failed_tests;
        }
    }
    
    // Test 17: Unmatched quotes
    {
        std::ofstream file("unmatched_quotes.ini");
        file << "key1=\"unmatched\n";     // Missing closing quote
        //file << "key2=value\"\n";         // Closing quote without opening
        file << "key3='single\n";         // Missing closing single quote
        file.close();

        ini::Parser parser;
        auto result = parser.load("unmatched_quotes.ini");

        bool check = (result == ini::Parser::ErrorCode::UnmatchedQuotes);
        if (!run_test("Test 17: Unmatched quotes", check)) {
            ++failed_tests;
        }
    }

    // Test 18: Trailing backslash in quoted value
    {
        std::ofstream file("trailing_backslash.ini");
        file << "key=\"value\\\\\"\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("trailing_backslash.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "key") == "value\\");
        if (!run_test("Test 18: Trailing backslash in quoted value", check)) {
            ++failed_tests;
        }
    }

    // Test 19: Empty quoted value
    {
        std::ofstream file("empty_quoted.ini");
        file << "key1=\"\"\n";
        file << "key2=''\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("empty_quoted.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "key1") == "" &&
                      parser.get("", "key2") == "");
        if (!run_test("Test 19: Empty quoted value", check)) {
            ++failed_tests;
        }
    }

    // Test 20: Whitespace-heavy input
    {
        std::ofstream file("whitespace_heavy.ini");
        file << "   key1   =   value1   \n";
        file << "[   section   ]\n";
        file << "   key2   =   value2   ; comment\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("whitespace_heavy.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "key1") == "value1" &&
                      parser.get("section", "key2") == "value2");
        if (!run_test("Test 20: Whitespace-heavy input", check)) {
            ++failed_tests;
        }
    }

    // Test 21: Multiple equals signs
    {
        std::ofstream file("multiple_equals.ini");
        file << "key=value=extra\n";
        file << "key2=\"quoted=value\"\n";
        file.close();

        ini::Parser parser;
        auto result = parser.load("multiple_equals.ini");
        bool check = (result == ini::Parser::ErrorCode::Success &&
                      parser.get("", "key") == "value=extra" &&
                      parser.get("", "key2") == "quoted=value");
        if (!run_test("Test 21: Multiple equals signs", check)) {
            ++failed_tests;
        }
    }

    std::cout << "Total failed tests: " << failed_tests << std::endl;
    return failed_tests > 0 ? 1 : 0;
}
