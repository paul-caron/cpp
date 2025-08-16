#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <optional>

/**
 * @class CmdLineParser
 * @brief Simple header-only command line argument parser for C++17.
 *
 * Supports named options with or without values, flags, and positional arguments.
 */
class CmdLineParser {
public:
    /**
     * @brief Parses argc/argv command line arguments.
     * @param argc Argument count.
     * @param argv Argument vector.
     */
    CmdLineParser(int argc, char* argv[]) {
        parse(argc, argv);
    }

    /**
     * @brief Checks if a flag or option was provided.
     * @param name Option name (without dashes).
     * @return true if present.
     */
    bool has(const std::string& name) const {
        return options.count(name) > 0;
    }

    /**
     * @brief Gets the value of an option.
     * @param name Option name (without dashes).
     * @return Optional string value, empty if not present or no value.
     */
    std::optional<std::string> get(const std::string& name) const {
        auto it = options.find(name);
        if (it != options.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Gets positional arguments (non-option arguments).
     * @return Vector of positional arguments.
     */
    const std::vector<std::string>& getPositionals() const {
        return positionals;
    }

    /**
     * @brief Prints all parsed options and positionals (debug).
     */
    void print() const {
        std::cout << "Options:\n";
        for (const auto& kv : options) {
            std::cout << "  " << kv.first << " = " << (kv.second.empty() ? "<flag>" : kv.second) << "\n";
        }
        std::cout << "Positional args:\n";
        for (const auto& p : positionals) {
            std::cout << "  " << p << "\n";
        }
    }

private:
    std::unordered_map<std::string, std::string> options;
    std::vector<std::string> positionals;

    void parse(int argc, char* argv[]) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg.size() > 1 && arg[0] == '-') {
                // Option or flag
                if (arg[1] == '-') {
                    // Long option: --name or --name=value
                    auto eq_pos = arg.find('=');
                    if (eq_pos != std::string::npos) {
                        std::string name = arg.substr(2, eq_pos - 2);
                        std::string value = arg.substr(eq_pos + 1);
                        options[name] = value;
                    } else {
                        std::string name = arg.substr(2);
                        // Check if next arg is a value (doesn't start with '-')
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                            options[name] = argv[++i];
                        } else {
                            // flag
                            options[name] = "";
                        }
                    }
                } else {
                    // Short option(s): -v or -abc (flags) or -o value
                    if (arg.size() == 2) {
                        std::string name = arg.substr(1, 1);
                        if (i + 1 < argc && argv[i + 1][0] != '-') {
                            options[name] = argv[++i];
                        } else {
                            options[name] = "";
                        }
                    } else {
                        // Multiple flags combined: -abc -> a, b, c flags
                        for (size_t j = 1; j < arg.size(); ++j) {
                            options[std::string(1, arg[j])] = "";
                        }
                    }
                }
            } else {
                // Positional argument
                positionals.push_back(arg);
            }
        }
    }
};
