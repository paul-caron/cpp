#pragma once

#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <random>

/**
 * @namespace fsu
 * @brief File System Utilities — a simple wrapper around std::filesystem (C++17).
 */
namespace fsu {

namespace fs = std::filesystem;

/**
 * @brief Checks whether a file or directory exists.
 * @param path The path to check.
 * @return true if the file or directory exists.
 */
inline bool exists(const fs::path& path) {
    return fs::exists(path);
}

/**
 * @brief Checks if a path points to a regular file.
 * @param path The path to check.
 * @return true if it is a regular file.
 */
inline bool isFile(const fs::path& path) {
    return fs::is_regular_file(path);
}

/**
 * @brief Checks if a path points to a directory.
 * @param path The path to check.
 * @return true if it is a directory.
 */
inline bool isDirectory(const fs::path& path) {
    return fs::is_directory(path);
}

/**
 * @brief Creates a directory (or directories) at the given path.
 * @param path The path to create.
 * @return true if created successfully, false otherwise.
 */
inline bool createDirectories(const fs::path& path) {
    return fs::create_directories(path);
}

/**
 * @brief Deletes a file or directory (recursively).
 * @param path The path to delete.
 * @return true if deletion was successful.
 */
inline bool remove(const fs::path& path) {
    return fs::remove_all(path) > 0;
}

/**
 * @brief Deletes a single file (non-recursive).
 * @param path The path to the file.
 * @return true if the file was deleted.
 */
inline bool deleteFile(const fs::path& path) {
    try {
        return fs::remove(path);
    } catch (const std::exception& e) {
        std::cerr << "Delete failed: " << e.what() << '\n';
        return false;
    }
}

/**
 * @brief Copies a file or directory to a new location.
 * @param from Source path.
 * @param to Destination path.
 * @param recursive If true, performs recursive copy.
 * @return true if copy was successful.
 */
inline bool copy(const fs::path& from, const fs::path& to, bool recursive = false) {
    try {
        if (recursive) {
            fs::copy(from, to, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        } else {
            fs::copy(from, to, fs::copy_options::overwrite_existing);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Copy failed: " << e.what() << '\n';
        return false;
    }
}

/**
 * @brief Moves or renames a file or directory.
 *
 * If the destination exists, the move will fail.
 * May fail across different mount points.
 *
 * @param from Source path.
 * @param to Destination path.
 * @return true if the move was successful.
 */
inline bool move(const fs::path& from, const fs::path& to) {
    try {
        fs::rename(from, to);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Move failed: " << e.what() << '\n';
        return false;
    }
}

/**
 * @brief Lists all files (optionally recursive) in a directory.
 * @param dir Directory path.
 * @param recursive If true, uses recursive iteration.
 * @return A vector of paths.
 */
inline std::vector<fs::path> listFiles(const fs::path& dir, bool recursive = false) {
    std::vector<fs::path> result;

    if (!fs::exists(dir) || !fs::is_directory(dir)) return result;

    try {
        if (recursive) {
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                result.push_back(entry.path());
            }
        } else {
            for (const auto& entry : fs::directory_iterator(dir)) {
                result.push_back(entry.path());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Directory listing failed: " << e.what() << '\n';
    }

    return result;
}

/**
 * @brief Reads the entire content of a text file.
 * @param path The file path.
 * @return String with file contents.
 */
inline std::string readFile(const fs::path& path) {
    std::ifstream in(path);
    if (!in) return "";
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

/**
 * @brief Writes a string to a file (overwrites).
 * @param path The file path.
 * @param content The content to write.
 * @return true on success.
 */
inline bool writeFile(const fs::path& path, const std::string& content) {
    std::ofstream out(path);
    if (!out) return false;
    out << content;
    return true;
}

/**
 * @brief Generates a unique temporary file name.
 * @param prefix Optional prefix string.
 * @return Path to a unique (non-existing) temp file.
 */
inline fs::path generateTempFile(const std::string& prefix = "tmp") {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);

    fs::path tempDir = fs::temp_directory_path();
    fs::path candidate;

    do {
        candidate = tempDir / (prefix + std::to_string(dis(gen)));
    } while (fs::exists(candidate));

    return candidate;
}

} // namespace fsu
