#ifndef MODULE_LOADER_HPP
#define MODULE_LOADER_HPP

#include <string>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

/**
 * @class ModuleLoader
 * @brief Cross-platform dynamic module (shared library) loader.
 *
 * Supports loading `.so`, `.dll`, and `.dylib` files at runtime.
 * Useful for implementing plugin systems or runtime extensibility.
 */
class ModuleLoader {
public:
    /**
     * @brief Default constructor. No module is loaded.
     */
    ModuleLoader() = default;

    /**
     * @brief Constructor that immediately attempts to load a module.
     * @param path Filesystem path to the shared library.
     */
    explicit ModuleLoader(const std::string& path) {
        load(path);
    }

    /**
     * @brief Destructor. Automatically unloads the module if loaded.
     */
    ~ModuleLoader() {
        unload();
    }

    // Copy operations are disabled
    ModuleLoader(const ModuleLoader&) = delete;
    ModuleLoader& operator=(const ModuleLoader&) = delete;

    /**
     * @brief Move constructor.
     * @param other ModuleLoader instance to move from.
     */
    ModuleLoader(ModuleLoader&& other) noexcept {
        handle = other.handle;
        other.handle = nullptr;
    }

    /**
     * @brief Move assignment operator.
     * @param other ModuleLoader instance to move from.
     * @return Reference to this instance.
     */
    ModuleLoader& operator=(ModuleLoader&& other) noexcept {
        if (this != &other) {
            unload();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }

    /**
     * @brief Loads a shared library from the given path.
     *
     * If another module was previously loaded, it will be unloaded first.
     *
     * @param path Filesystem path to the shared library.
     * @return true if the library was successfully loaded, false otherwise.
     */
    bool load(const std::string& path) {
        unload(); // In case already loaded

    #ifdef _WIN32
        handle = LoadLibraryA(path.c_str());
        if (!handle) {
            std::cerr << "Failed to load module: " << path << "\n";
        }
    #else
        handle = dlopen(path.c_str(), RTLD_LAZY);
        if (!handle) {
            std::cerr << "Failed to load module: " << path << "\n";
            std::cerr << "Error: " << dlerror() << "\n";
        }
    #endif
        return handle != nullptr;
    }

    /**
     * @brief Unloads the currently loaded module, if any.
     */
    void unload() {
        if (!handle) return;

    #ifdef _WIN32
        FreeLibrary((HMODULE)handle);
    #else
        dlclose(handle);
    #endif
        handle = nullptr;
    }

    /**
     * @brief Checks whether a module is currently loaded.
     * @return true if a module is loaded, false otherwise.
     */
    bool isLoaded() const {
        return handle != nullptr;
    }

    /**
     * @brief Retrieves a symbol (function or variable) from the loaded module.
     *
     * You must know the correct type for the symbol you are retrieving.
     *
     * @tparam T Function pointer or variable type.
     * @param name Name of the exported symbol.
     * @return Pointer to the symbol, or nullptr if not found or module is not loaded.
     */
    template<typename T>
    T getSymbol(const std::string& name) const {
        if (!handle) return nullptr;

    #ifdef _WIN32
        return reinterpret_cast<T>(GetProcAddress((HMODULE)handle, name.c_str()));
    #else
        return reinterpret_cast<T>(dlsym(handle, name.c_str()));
    #endif
    }

private:
#ifdef _WIN32
    HMODULE handle = nullptr;  ///< Windows handle to the loaded module.
#else
    void* handle = nullptr;    ///< POSIX handle to the loaded module.
#endif
};

#endif //MODULE_LOADER_HPP



/*
//example use case
#include "module_loader.hpp"
#include <iostream>

int main() {
    const char* libPath = "/lib/x86_64-linux-gnu/libm.so.6";

    ModuleLoader loader;
    if (!loader.load(libPath)) {
        std::cerr << "Failed to load math library.\n";
        return 1;
    }

    // Load 'cos' function: double cos(double)
    using CosFunc = double(*)(double);
    CosFunc cosFunc = loader.getSymbol<CosFunc>("cos");

    if (!cosFunc) {
        std::cerr << "Failed to find symbol 'cos'\n";
        return 1;
    }

    // Call the function
    double input = 0.5;
    double result = cosFunc(input);
    std::cout << "cos(" << input << ") = " << result << std::endl;

    return 0;
}

*/
